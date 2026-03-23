// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI

#include "ocudu/adt/byte_buffer.h"
#include "ocudu/adt/detail/byte_segment_list.h"
#include <gtest/gtest.h>
#include <numeric>

using namespace ocudu;
using namespace ocudu::detail;

namespace {

class shared_byte_segment_list_test : public ::testing::Test
{
protected:
  byte_buffer_memory_resource& pool = get_default_byte_buffer_segment_pool();

  // Convenience: make a list and assert success.
  shared_byte_segment_list make_list(size_t seg_size_hint = 128)
  {
    return shared_byte_segment_list::make(seg_size_hint, pool).value();
  }
};

TEST_F(shared_byte_segment_list_test, default_constructed_is_empty)
{
  shared_byte_segment_list list;
  ASSERT_TRUE(list.empty());
  ASSERT_EQ(list.length(), 0U);
  ASSERT_TRUE(list.tail().empty());

  size_t count = 0;
  for (span<uint8_t> seg : list) {
    (void)seg;
    ++count;
  }
  ASSERT_EQ(count, 0U);
}

TEST_F(shared_byte_segment_list_test, make_creates_initial_segment)
{
  auto list = make_list();

  ASSERT_FALSE(list.empty());
  ASSERT_FALSE(list.tail().empty());
  ASSERT_GE(list.tail().size(), 1U);
  ASSERT_EQ(list.length(), list.tail().size());

  size_t count = 0;
  for (span<uint8_t> seg : list) {
    ASSERT_FALSE(seg.empty());
    ++count;
  }
  ASSERT_EQ(count, 1U);
}

TEST_F(shared_byte_segment_list_test, initial_segment_write_and_read)
{
  auto list = make_list();

  span<uint8_t> seg = list.tail();
  ASSERT_GE(seg.size(), 1U);
  std::iota(seg.begin(), seg.end(), uint8_t{0});

  // Verify via const range.
  const shared_byte_segment_list& clist = list;
  size_t                          idx   = 0;
  for (span<const uint8_t> cseg : clist) {
    for (size_t i = 0; i < cseg.size(); ++i) {
      ASSERT_EQ(cseg[i], static_cast<uint8_t>(idx++));
    }
  }
}

TEST_F(shared_byte_segment_list_test, append_segment_adds_segment)
{
  auto list = make_list();
  ASSERT_TRUE(list.append_segment(128));

  ASSERT_FALSE(list.empty());

  size_t count = 0;
  for (span<uint8_t> seg : list) {
    ASSERT_FALSE(seg.empty());
    ++count;
  }
  // Initial segment from make + 1 appended.
  ASSERT_EQ(count, 2U);
  ASSERT_EQ(list.length(), list.tail().size() + (*list.begin()).size());
}

TEST_F(shared_byte_segment_list_test, multi_segment_append)
{
  auto             list         = make_list();
  constexpr size_t num_appended = 3;
  for (size_t i = 0; i < num_appended; ++i) {
    ASSERT_TRUE(list.append_segment(128));
  }

  ASSERT_FALSE(list.empty());

  size_t count      = 0;
  size_t total_size = 0;
  for (span<uint8_t> seg : list) {
    ASSERT_FALSE(seg.empty());
    total_size += seg.size();
    ++count;
  }
  // Initial segment from make + num_appended.
  ASSERT_EQ(count, num_appended + 1);
  ASSERT_EQ(list.length(), total_size);
  ASSERT_FALSE(list.tail().empty());
}

TEST_F(shared_byte_segment_list_test, clear_resets_to_empty)
{
  auto list = make_list();
  ASSERT_FALSE(list.empty());

  list.clear();
  ASSERT_TRUE(list.empty());
  ASSERT_EQ(list.length(), 0U);
  ASSERT_TRUE(list.tail().empty());
}

TEST_F(shared_byte_segment_list_test, shallow_copy_shares_control_block)
{
  auto list = make_list();

  span<uint8_t> seg = list.tail();
  std::fill(seg.begin(), seg.end(), uint8_t{0xab});

  shared_byte_segment_list copy = list;
  ASSERT_FALSE(copy.empty());
  ASSERT_EQ(copy.length(), list.length());

  // Both should see the same bytes (shared ownership).
  size_t total = 0;
  for (span<const uint8_t> cseg : copy) {
    for (uint8_t byte : cseg) {
      ASSERT_EQ(byte, uint8_t{0xab});
      ++total;
    }
  }
  ASSERT_EQ(total, list.length());
}

TEST_F(shared_byte_segment_list_test, original_still_valid_after_copy_destroyed)
{
  auto          list = make_list();
  span<uint8_t> seg  = list.tail();
  std::fill(seg.begin(), seg.end(), uint8_t{0xcd});

  {
    const shared_byte_segment_list copy = list;
    ASSERT_FALSE(copy.empty());
  } // copy destroyed here; ref count drops from 2 to 1.

  ASSERT_FALSE(list.empty());
  for (span<const uint8_t> cseg : list) {
    for (uint8_t byte : cseg) {
      ASSERT_EQ(byte, uint8_t{0xcd});
    }
  }
}

TEST_F(shared_byte_segment_list_test, move_constructor_leaves_source_empty)
{
  auto src = make_list();
  ASSERT_TRUE(src.append_segment(128));

  shared_byte_segment_list dst{std::move(src)};
  ASSERT_TRUE(src.empty());
  ASSERT_FALSE(dst.empty());
}

TEST_F(shared_byte_segment_list_test, move_assignment_leaves_source_empty)
{
  auto src = make_list();
  ASSERT_TRUE(src.append_segment(128));

  auto dst = make_list();
  dst      = std::move(src);

  ASSERT_TRUE(src.empty());
  ASSERT_FALSE(dst.empty());
}

TEST_F(shared_byte_segment_list_test, move_assignment_drops_previous_content)
{
  auto   a     = make_list();
  size_t a_len = a.length();

  auto b = make_list();
  ASSERT_TRUE(b.append_segment(128));
  ASSERT_TRUE(b.append_segment(128));

  b = std::move(a);

  ASSERT_EQ(b.length(), a_len);
  size_t count = 0;
  for (span<uint8_t> seg : b) {
    (void)seg;
    ++count;
  }
  ASSERT_EQ(count, 1U);
}

TEST_F(shared_byte_segment_list_test, default_ctor_append_segment_uses_malloc_pool)
{
  shared_byte_segment_list list;
  ASSERT_TRUE(list.empty());

  ASSERT_TRUE(list.append_segment(128));
  ASSERT_FALSE(list.empty());
  ASSERT_FALSE(list.tail().empty());
  ASSERT_GE(list.tail().size(), 1U);

  ASSERT_TRUE(list.append_segment(64));
  size_t count = 0;
  for (span<uint8_t> seg : list) {
    ASSERT_FALSE(seg.empty());
    ++count;
  }
  ASSERT_EQ(count, 2U);
}

TEST_F(shared_byte_segment_list_test, byte_segment_list_typedef_is_non_owning)
{
  // byte_segment_list is the non-owning intrusive list type.
  static_assert(std::is_same_v<byte_segment_list, intrusive_forward_list<byte_segment_node>>,
                "byte_segment_list must be an alias for intrusive_forward_list<byte_segment_node>");

  byte_segment_list list;
  ASSERT_TRUE(list.empty());
}

} // namespace
