/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "ocudu/adt/stable_id_map.h"
#include <gtest/gtest.h>

using namespace ocudu;

TEST(stable_id_map_test, empty_table)
{
  stable_id_map<int> table;

  ASSERT_TRUE(table.empty());
  ASSERT_EQ(table.size(), 0);
  ASSERT_EQ(table.capacity(), 0);
  ASSERT_FALSE(table.contains(stable_id_t{0}));
}

TEST(stable_id_map_test, insert_one_object)
{
  stable_id_map<int> table;

  stable_id_t id = table.insert(42);
  ASSERT_EQ(id.value(), 0);
  ASSERT_FALSE(table.empty());
  ASSERT_EQ(table.size(), 1);
  ASSERT_GE(table.capacity(), 1);
  ASSERT_TRUE(table.contains(id));
  ASSERT_EQ(table.at(id), 42);
  ASSERT_EQ(table[id], 42);

  auto unsorted = table.unsorted();
  ASSERT_EQ(unsorted.size(), 1);
  ASSERT_EQ(unsorted[0], 42);
}

TEST(stable_id_map_test, insert_and_rem_one_object)
{
  stable_id_map<int> table;
  auto               id = table.insert(42);
  ASSERT_TRUE(table.erase(id));
  ASSERT_TRUE(table.empty());
  ASSERT_EQ(table.size(), 0);
  ASSERT_FALSE(table.contains(id));
  ASSERT_TRUE(table.unsorted().empty());
}

TEST(stable_id_map_test, insert_two)
{
  stable_id_map<int> table;
  auto               id0 = table.insert(42);
  auto               id1 = table.insert(43);
  ASSERT_NE(id0, id1);

  ASSERT_FALSE(table.empty());
  ASSERT_EQ(table.size(), 2);
  ASSERT_TRUE(table.contains(id0));
  ASSERT_TRUE(table.contains(id1));
  ASSERT_EQ(table[id0], 42);
  ASSERT_EQ(table[id1], 43);
}

TEST(stable_id_map_test, insert_and_rem_two)
{
  stable_id_map<int> table;
  auto               id0 = table.insert(42);
  auto               id1 = table.insert(43);
  ASSERT_TRUE(table.erase(id0));

  ASSERT_FALSE(table.empty());
  ASSERT_EQ(table.size(), 1);
  ASSERT_FALSE(table.contains(id0));
  ASSERT_TRUE(table.contains(id1));
  ASSERT_EQ(table[id1], 43);

  ASSERT_TRUE(table.erase(id1));
  ASSERT_TRUE(table.empty());
  ASSERT_EQ(table.size(), 0);
  ASSERT_FALSE(table.contains(id0));
  ASSERT_FALSE(table.contains(id1));
}

TEST(stable_id_map_test, insert_two_rem_and_insert)
{
  stable_id_map<int> table;
  auto               id0 = table.insert(42);
  auto               id1 = table.insert(43);
  ASSERT_TRUE(table.erase(id0));
  ASSERT_FALSE(table.contains(id0));
  auto id2 = table.insert(44);

  ASSERT_TRUE(table.contains(id2));
  ASSERT_EQ(id2, id0) << "row_id should be reused";
  ASSERT_NE(id1, id2);
  ASSERT_EQ(table.size(), 2);
  ASSERT_EQ(table[id1], 43) << "row_id must stay stable";
  ASSERT_EQ(table[id2], 44);
  ASSERT_NE(id1, id2);
}

TEST(stable_id_map_test, insert_three_rem_two_and_insert)
{
  stable_id_map<int> table;
  auto               id0 = table.insert(42);
  auto               id1 = table.insert(43);
  auto               id2 = table.insert(44);
  ASSERT_TRUE(table.erase(id0));
  ASSERT_TRUE(table.erase(id1));
  ASSERT_FALSE(table.contains(id0));
  ASSERT_FALSE(table.contains(id1));
  auto id3 = table.insert(45);

  ASSERT_TRUE(table.contains(id2));
  ASSERT_TRUE(table.contains(id3));
}

TEST(stable_id_map_test, iterator)
{
  stable_id_map<int> table;
  ASSERT_EQ(table.begin(), table.end());

  table.insert(42);
  auto id1 = table.insert(43);

  auto it = table.begin();
  ASSERT_NE(it, table.end());
  ASSERT_EQ(*it, 42);
  ++it;
  ASSERT_NE(it, table.end());
  ASSERT_EQ(*it, table[id1]);
  ASSERT_EQ(*it, 43);
  ++it;
  ASSERT_EQ(it, table.end());

  table.erase(table.begin());
  ASSERT_EQ(table.size(), 1);
  it = table.begin();
  ASSERT_NE(it, table.end());
  ASSERT_EQ(*it, table[id1]);
  ASSERT_EQ(*it, 43);

  table.erase(it);
  ASSERT_EQ(table.end(), table.begin());
}

TEST(stable_id_map_test, reserve_and_clear_resets_state)
{
  // This test guards the reset logic after bulk cleanup.
  stable_id_map<int> table;
  table.reserve(10);
  ASSERT_GE(table.capacity(), 10);

  auto id0 = table.insert(42);
  table.insert(43);
  ASSERT_EQ(table.size(), 2);

  table.clear();
  ASSERT_TRUE(table.empty());
  ASSERT_EQ(table.size(), 0);
  ASSERT_GE(table.capacity(), 10);

  auto id2 = table.insert(44);
  ASSERT_EQ(id2, id0) << "row_id should be reused";
  ASSERT_EQ(table.size(), 1);
  ASSERT_EQ(table[id2], 44);
  ASSERT_FALSE(false);
}

TEST(stable_id_map_test, erase_invalid_row_id_is_noop)
{
  stable_id_map<int> table;
  auto               id0 = table.insert(42);
  table.insert(43);
  auto id2 = table.insert(44);

  // Erasing invalid row should be no-op.
  ASSERT_EQ(table.size(), 3);
  ASSERT_FALSE(table.erase(stable_id_t{id2.value() + 1}));
  ASSERT_EQ(table.size(), 3);
  ASSERT_TRUE(table.contains(id0));
  ASSERT_TRUE(table.contains(id2));

  // Erasing again should be a noop.
  ASSERT_TRUE(table.erase(id0));
  ASSERT_EQ(table.size(), 2);
  ASSERT_FALSE(table.contains(id0));
  ASSERT_TRUE(table.contains(id2));
  ASSERT_FALSE(table.erase(id0));
  ASSERT_EQ(table.size(), 2);
  ASSERT_TRUE(table.contains(id2));
  ASSERT_FALSE(table.contains(id0));
}

TEST(stable_id_map_test, move_only_columns)
{
  stable_id_map<std::unique_ptr<int>> table;
  auto                                ptr1     = std::make_unique<int>(42);
  auto*                               ptr1_raw = ptr1.get();
  auto                                ptr2     = std::make_unique<int>(43);
  auto*                               ptr2_raw = ptr2.get();
  auto                                id0      = table.insert(std::move(ptr1));
  auto                                id1      = table.insert(std::move(ptr2));
  ASSERT_EQ(table[id0].get(), ptr1_raw);
  ASSERT_EQ(table[id1].get(), ptr2_raw);
  ASSERT_EQ(*table[id0], 42);
  ASSERT_EQ(*table[id1], 43);
  ASSERT_EQ(ptr1, nullptr);
  ASSERT_EQ(ptr2, nullptr);
  table.erase(id0);
  ASSERT_FALSE(table.contains(id0));
  ASSERT_EQ(*table[id1], 43);
}
