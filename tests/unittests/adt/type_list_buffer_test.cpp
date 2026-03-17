// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI

#include "ocudu/adt/type_list_buffer.h"
#include <gtest/gtest.h>
#include <vector>

using namespace ocudu;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

/// Trivially copyable small struct.
struct small_pod {
  uint8_t x;
  bool    operator==(const small_pod& o) const { return x == o.x; }
};

/// Larger trivially copyable struct.
struct large_pod {
  uint64_t a;
  uint64_t b;
  bool     operator==(const large_pod& o) const { return a == o.a && b == o.b; }
};

/// Struct with an 8-byte-aligned member.
struct aligned8 {
  alignas(8) uint8_t data[8];
  bool operator==(const aligned8& o) const { return std::memcmp(data, o.data, 8) == 0; }
};

/// Non-trivially destructible type (tracks construction / destruction counts).
struct counted {
  inline static int constructions = 0;
  inline static int destructions  = 0;

  int value;

  explicit counted(int v) : value(v) { ++constructions; }
  counted(const counted& o) : value(o.value) { ++constructions; }
  counted(counted&& o) noexcept : value(o.value) { ++constructions; }
  ~counted() { ++destructions; }

  bool operator==(const counted& o) const { return value == o.value; }

  static void reset() { constructions = destructions = 0; }
};

struct two_arg_pod {
  uint32_t x;
  uint32_t y;
  bool     operator==(const two_arg_pod& o) const { return x == o.x && y == o.y; }
};

// ---------------------------------------------------------------------------
// Compile-time checks
// ---------------------------------------------------------------------------

static_assert(std::is_default_constructible_v<type_list_buffer<int, float>>);
static_assert(std::is_move_constructible_v<type_list_buffer<int, float>>);
static_assert(std::is_copy_constructible_v<type_list_buffer<int, float>>);

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

TEST(type_list_buffer, default_ctor_is_empty)
{
  type_list_buffer<int, float, double> buf;
  EXPECT_TRUE(buf.empty());
  EXPECT_EQ(buf.size(), 0u);
  EXPECT_EQ(buf.byte_size(), 0u);
}

TEST(type_list_buffer, push_single_type_and_visit)
{
  type_list_buffer<int> buf;
  buf.push(42);
  ASSERT_EQ(buf.size(), 1u);

  std::vector<int> visited;
  buf.for_each([&](int v) { visited.push_back(v); });
  ASSERT_EQ(visited.size(), 1u);
  EXPECT_EQ(visited[0], 42);
}

TEST(type_list_buffer, push_multiple_types_preserves_order)
{
  type_list_buffer<small_pod, int, large_pod> buf;

  buf.push(small_pod{7});
  buf.push(1234);
  buf.push(large_pod{0xAA, 0xBB});
  buf.push(small_pod{99});
  buf.push(-1);

  EXPECT_EQ(buf.size(), 5u);

  // Use a reference-capturing lambda so accumulated state is visible after for_each returns.
  std::vector<int> kinds; // 0=small_pod, 1=int, 2=large_pod
  buf.for_each([&kinds](const auto& obj) {
    using T = std::decay_t<decltype(obj)>;
    if constexpr (std::is_same_v<T, small_pod>) {
      kinds.push_back(0);
    } else if constexpr (std::is_same_v<T, int>) {
      kinds.push_back(1);
    } else if constexpr (std::is_same_v<T, large_pod>) {
      kinds.push_back(2);
    }
  });
  ASSERT_EQ(kinds.size(), 5u);
  EXPECT_EQ(kinds[0], 0);
  EXPECT_EQ(kinds[1], 1);
  EXPECT_EQ(kinds[2], 2);
  EXPECT_EQ(kinds[3], 0);
  EXPECT_EQ(kinds[4], 1);
}

TEST(type_list_buffer, visited_values_are_correct)
{
  type_list_buffer<small_pod, int, large_pod> buf;
  buf.push(small_pod{3});
  buf.push(-7);
  buf.push(large_pod{100, 200});

  small_pod sp{};
  int       iv{};
  large_pod lp{};

  buf.for_each([&](const auto& obj) {
    using T = std::decay_t<decltype(obj)>;
    if constexpr (std::is_same_v<T, small_pod>) {
      sp = obj;
    } else if constexpr (std::is_same_v<T, int>) {
      iv = obj;
    } else if constexpr (std::is_same_v<T, large_pod>) {
      lp = obj;
    }
  });

  EXPECT_EQ(sp, (small_pod{3}));
  EXPECT_EQ(iv, -7);
  EXPECT_EQ(lp, (large_pod{100, 200}));
}

TEST(type_list_buffer, smaller_types_occupy_less_space_than_largest)
{
  // A single small_pod (1 byte) + uint8_t type index = 2 bytes.
  // A single large_pod (16 bytes) + uint8_t type index = 17 bytes.
  // Together: at most 2 + 17 = 19 bytes (ignoring any 1-byte alignment padding).
  // A variant<small_pod, large_pod>[2] would need 2 * (16 + 1) = 34 bytes.
  type_list_buffer<small_pod, large_pod> buf;
  buf.push(small_pod{1});
  buf.push(large_pod{0, 0});

  EXPECT_LT(buf.byte_size(), 2 * (sizeof(large_pod) + sizeof(uint8_t)));
}

TEST(type_list_buffer, alignment_is_respected)
{
  type_list_buffer<uint8_t, aligned8> buf;
  buf.push(uint8_t{0xAB});
  buf.emplace<aligned8>(aligned8{0, 1, 2, 3, 4, 5, 6, 7});

  aligned8 out{};
  buf.for_each([&](const auto& obj) {
    if constexpr (std::is_same_v<std::decay_t<decltype(obj)>, aligned8>) {
      out = obj;
    }
  });

  EXPECT_EQ(out, (aligned8{0, 1, 2, 3, 4, 5, 6, 7}));
}

TEST(type_list_buffer, mutable_for_each_allows_modification)
{
  type_list_buffer<int, float> buf;
  buf.push(10);
  buf.push(2.5f);
  buf.push(20);

  buf.for_each([](auto& obj) {
    using T = std::decay_t<decltype(obj)>;
    if constexpr (std::is_same_v<T, int>) {
      obj *= 2;
    }
  });

  std::vector<int> ints;
  buf.for_each([&](const auto& obj) {
    if constexpr (std::is_same_v<std::decay_t<decltype(obj)>, int>) {
      ints.push_back(obj);
    }
  });

  ASSERT_EQ(ints.size(), 2u);
  EXPECT_EQ(ints[0], 20);
  EXPECT_EQ(ints[1], 40);
}

TEST(type_list_buffer, clear_resets_buffer)
{
  type_list_buffer<int, double> buf;
  buf.push(1);
  buf.push(2.0);
  EXPECT_EQ(buf.size(), 2u);

  buf.clear();
  EXPECT_TRUE(buf.empty());
  EXPECT_EQ(buf.size(), 0u);
  EXPECT_EQ(buf.byte_size(), 0u);

  // Re-use after clear.
  buf.push(99);
  EXPECT_EQ(buf.size(), 1u);
}

TEST(type_list_buffer, non_trivial_destructors_are_called_on_clear)
{
  counted::reset();
  {
    type_list_buffer<counted, int> buf;
    buf.emplace<counted>(1);
    buf.push(0);
    buf.emplace<counted>(2);
    EXPECT_EQ(counted::constructions, 2);
    EXPECT_EQ(counted::destructions, 0);

    buf.clear();
    EXPECT_EQ(counted::destructions, 2);
  }
  // No extra destructions from the (now-empty) buffer's destructor.
  EXPECT_EQ(counted::destructions, 2);
}

TEST(type_list_buffer, non_trivial_destructors_called_on_scope_exit)
{
  counted::reset();
  {
    type_list_buffer<counted, int> buf;
    buf.emplace<counted>(10);
    buf.emplace<counted>(20);
    EXPECT_EQ(counted::constructions, 2);
  }
  EXPECT_EQ(counted::destructions, 2);
}

TEST(type_list_buffer, copy_ctor_produces_independent_copy)
{
  type_list_buffer<int, float> original;
  original.push(1);
  original.push(3.14f);
  original.push(2);

  type_list_buffer<int, float> copy = original;

  EXPECT_EQ(copy.size(), original.size());

  // Modify original; copy must be unaffected.
  original.for_each([](auto& obj) {
    if constexpr (std::is_same_v<std::decay_t<decltype(obj)>, int>) {
      obj = -1;
    }
  });

  std::vector<int> copy_ints;
  copy.for_each([&](const auto& obj) {
    if constexpr (std::is_same_v<std::decay_t<decltype(obj)>, int>) {
      copy_ints.push_back(obj);
    }
  });

  ASSERT_EQ(copy_ints.size(), 2u);
  EXPECT_EQ(copy_ints[0], 1);
  EXPECT_EQ(copy_ints[1], 2);
}

TEST(type_list_buffer, copy_ctor_calls_copy_constructor_for_non_trivial_types)
{
  counted::reset();
  {
    type_list_buffer<counted, int> src;
    src.emplace<counted>(5);
    src.push(0);

    int constructions_before_copy = counted::constructions;

    type_list_buffer<counted, int> dst = src;
    EXPECT_EQ(counted::constructions, constructions_before_copy + 1);

    // Both src and dst hold one counted; both destructors must fire.
  }
  EXPECT_EQ(counted::destructions, counted::constructions);
}

TEST(type_list_buffer, move_ctor_transfers_ownership)
{
  type_list_buffer<int, double> buf;
  buf.push(7);
  buf.push(3.0);
  const size_t orig_size    = buf.size();
  const size_t orig_byte_sz = buf.byte_size();

  type_list_buffer<int, double> moved = std::move(buf);

  EXPECT_EQ(moved.size(), orig_size);
  EXPECT_EQ(moved.byte_size(), orig_byte_sz);
}

TEST(type_list_buffer, emplace_constructs_in_place)
{
  // Verifies that emplace forwards arguments to the object constructor.
  type_list_buffer<two_arg_pod, int> buf;
  buf.emplace<two_arg_pod>(two_arg_pod{42u, 99u});
  buf.push(0);

  two_arg_pod result{};
  buf.for_each([&](const auto& obj) {
    if constexpr (std::is_same_v<std::decay_t<decltype(obj)>, two_arg_pod>) {
      result = obj;
    }
  });
  EXPECT_EQ(result, (two_arg_pod{42u, 99u}));
}

TEST(type_list_buffer, large_number_of_elements)
{
  type_list_buffer<uint8_t, uint32_t> buf;
  constexpr int                       num_elements = 1000;
  for (int i = 0; i < num_elements; ++i) {
    if (i % 2 == 0) {
      buf.push(static_cast<uint8_t>(i & 0xFF));
    } else {
      buf.push(static_cast<uint32_t>(i));
    }
  }
  EXPECT_EQ(buf.size(), static_cast<size_t>(num_elements));

  int count = 0;
  buf.for_each([&](const auto&) { ++count; });
  EXPECT_EQ(count, num_elements);
}
