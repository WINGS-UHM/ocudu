// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI

#include "ocudu/adt/type_list_segment_buffer.h"
#include <gtest/gtest.h>
#include <vector>

using namespace ocudu;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

struct counted_seg {
  inline static int constructions = 0;
  inline static int destructions  = 0;

  int value;

  explicit counted_seg(int v) : value(v) { ++constructions; }
  counted_seg(const counted_seg& o) : value(o.value) { ++constructions; }
  counted_seg(counted_seg&& o) noexcept : value(o.value) { ++constructions; }
  ~counted_seg() { ++destructions; }

  bool operator==(const counted_seg& o) const { return value == o.value; }

  static void reset() { constructions = destructions = 0; }
};

// ---------------------------------------------------------------------------
// Compile-time checks
// ---------------------------------------------------------------------------

static_assert(!std::is_copy_constructible_v<type_list_segment_buffer<int, float>>);
static_assert(std::is_move_constructible_v<type_list_segment_buffer<int, float>>);
static_assert(std::is_default_constructible_v<type_list_segment_buffer<int, float>>);

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

TEST(type_list_segment_buffer_test, basic_push_and_emplace)
{
  type_list_segment_buffer<int, float> buf;

  int*   ip = buf.emplace<int>(42);
  float* fp = buf.push(1.5f);

  ASSERT_NE(ip, nullptr);
  ASSERT_NE(fp, nullptr);
  ASSERT_EQ(*ip, 42);
  ASSERT_FLOAT_EQ(*fp, 1.5f);
  ASSERT_EQ(buf.size(), 2u);
}

TEST(type_list_segment_buffer_test, for_each_visits_in_order)
{
  type_list_segment_buffer<int, float> buf;

  buf.emplace<int>(1);
  buf.push(2.0f);
  buf.emplace<int>(3);
  buf.push(4.0f);

  std::vector<int> order; // 0 = int, 1 = float
  buf.for_each([&](const auto& obj) {
    using T = std::decay_t<decltype(obj)>;
    order.push_back(std::is_same_v<T, int> ? 0 : 1);
  });

  ASSERT_EQ(order, (std::vector<int>{0, 1, 0, 1}));
}

TEST(type_list_segment_buffer_test, segment_overflow_creates_new_segment)
{
  type_list_segment_buffer<int> buf;
  constexpr int                 num_elements = 500; // guaranteed to span multiple segments
  for (int i = 0; i < num_elements; ++i) {
    ASSERT_NE(buf.emplace<int>(i), nullptr);
  }
  ASSERT_EQ(buf.size(), static_cast<size_t>(num_elements));
  int last = -1;
  buf.for_each([&](const int& v) { last = v; });
  ASSERT_EQ(last, num_elements - 1);
}

TEST(type_list_segment_buffer_test, clear_destroys_and_allows_reuse)
{
  counted_seg::reset();
  {
    type_list_segment_buffer<counted_seg, int> buf;

    buf.emplace<counted_seg>(1);
    buf.push(0);
    buf.emplace<counted_seg>(2);

    ASSERT_EQ(counted_seg::constructions - counted_seg::destructions, 2);
    const int dtors_before = counted_seg::destructions;

    buf.clear();
    ASSERT_EQ(counted_seg::destructions, dtors_before + 2);
    ASSERT_TRUE(buf.empty());

    auto* p = buf.emplace<counted_seg>(99);
    ASSERT_NE(p, nullptr);
    ASSERT_EQ(p->value, 99);
    ASSERT_EQ(buf.size(), 1u);
  }
  ASSERT_EQ(counted_seg::destructions, counted_seg::constructions);
}

TEST(type_list_segment_buffer_test, move_transfers_elements)
{
  type_list_segment_buffer<int, float> src;
  src.emplace<int>(10);
  src.push(2.5f);
  src.emplace<int>(20);

  type_list_segment_buffer<int, float> dst(std::move(src));

  ASSERT_TRUE(src.empty());
  ASSERT_EQ(dst.size(), 3u);

  std::vector<int> vals;
  dst.for_each([&](const auto& v) {
    if constexpr (std::is_same_v<std::decay_t<decltype(v)>, int>) {
      vals.push_back(v);
    }
  });
  ASSERT_EQ(vals, (std::vector<int>{10, 20}));
}

TEST(type_list_segment_buffer_test, destructor_calls_dtors)
{
  counted_seg::reset();
  {
    type_list_segment_buffer<counted_seg, int> buf;
    buf.emplace<counted_seg>(1);
    buf.emplace<counted_seg>(2);
    buf.emplace<counted_seg>(3);
    ASSERT_EQ(counted_seg::constructions, 3);
  }
  ASSERT_EQ(counted_seg::destructions, 3);
}

TEST(type_list_segment_buffer_test, shallow_copy_shares_elements)
{
  type_list_segment_buffer<int, float> src;
  src.emplace<int>(7);
  src.push(3.14f);

  // Shallow copy: both see the same elements.
  type_list_segment_buffer<int, float> c = src.copy();
  ASSERT_EQ(c.size(), 2u);

  // Elements pushed after the copy are visible through both handles.
  src.emplace<int>(99);
  ASSERT_EQ(src.size(), 3u);
  ASSERT_EQ(c.size(), 3u);
}
