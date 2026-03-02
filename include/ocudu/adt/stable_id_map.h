// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI

#pragma once

#include "ocudu/adt/span.h"
#include "ocudu/adt/strong_type.h"
#include "ocudu/support/ocudu_assert.h"
#include <limits>
#include <vector>

namespace ocudu {

/// Type representing an ID of a stable_id_map.
struct stable_id_tag;
using stable_id_t = strong_type<uint32_t, stable_id_tag, strong_equality, strong_comparison>;

/// \brief List of objects that have a stable ID and are represented contiguously in memory, but their address in the
/// internal storage may change.
template <typename T>
class stable_id_map
{
  struct sentinel {};

  /// Iterator of objects. The iterator is not sorted by stable ID.
  template <typename TableType>
  class iter_impl
  {
    constexpr static bool is_const = std::is_const_v<TableType>;

  public:
    using value_type        = T;
    using difference_type   = std::ptrdiff_t;
    using reference         = std::conditional_t<is_const, const T&, T&>;
    using pointer           = std::conditional_t<is_const, const T*, T*>;
    using iterator_category = std::forward_iterator_tag;

    iter_impl() = default;
    iter_impl(TableType& table_, size_t offset_) : parent(&table_), offset(offset_) {}

    operator iter_impl<const stable_id_map>() const noexcept { return iter_impl<const stable_id_map>{*parent, offset}; }

    reference       operator*() { return parent->objs[offset]; }
    const reference operator*() const { return parent->objs[offset]; }
    pointer         operator->() { return &parent->objs[offset]; }
    const pointer   operator->() const { return &parent->objs[offset]; }

    iter_impl& operator++()
    {
      ++offset;
      return *this;
    }
    iter_impl operator++(int)
    {
      iter_impl tmp = *this;
      ++offset;
      return tmp;
    }

    bool        operator==(const iter_impl& other) const { return offset == other.offset and parent == other.parent; }
    bool        operator!=(const iter_impl& other) const { return not(*this == other); }
    bool        operator==(sentinel /* unused */) const { return offset == parent->size(); }
    bool        operator!=(sentinel /* unused */) const { return offset != parent->size(); }
    friend bool operator==(sentinel /* unused */, const iter_impl& rhs) { return rhs.offset == rhs.parent->size(); }
    friend bool operator!=(sentinel /* unused */, const iter_impl& rhs) { return rhs.offset != rhs.parent->size(); }

  private:
    friend class stable_id_map<T>;

    TableType* parent = nullptr;
    unsigned   offset = 0;
  };

public:
  using iterator       = iter_impl<stable_id_map>;
  using const_iterator = iter_impl<const stable_id_map>;

  /// Retrieves the number of objects currently stored.
  [[nodiscard]] size_t size() const { return index_reverse_map.size(); }

  /// Checks if the table contains any objects.
  [[nodiscard]] bool empty() const { return index_reverse_map.empty(); }

  /// Gets pre-reserved space.
  [[nodiscard]] size_t capacity() const { return index_reverse_map.capacity(); }

  /// Checks whether the table contains object with provided ID.
  [[nodiscard]] bool contains(stable_id_t id) const
  {
    if (id.value() >= index_map.size()) {
      return false;
    }
    const unsigned offset = index_map[id.value()];
    if (offset >= index_reverse_map.size()) {
      return false;
    }
    return index_reverse_map[offset] == id;
  }

  /// Retrieves an object with the provided ID.
  T& at(stable_id_t id)
  {
    ocudu_assert(contains(id), "Invalid stable ID");
    return objs[index_map[id.value()]];
  }
  T& operator[](stable_id_t id)
  {
    ocudu_assert(contains(id), "Invalid stable ID");
    return objs[index_map[id.value()]];
  }
  const T& at(stable_id_t id) const
  {
    ocudu_assert(contains(id), "Invalid stable ID");
    return objs[index_map[id.value()]];
  }
  const T& operator[](stable_id_t id) const
  {
    ocudu_assert(contains(id), "Invalid stable ID");
    return objs[index_map[id.value()]];
  }

  /// Get the internal offset of the object with given ID.
  unsigned get_offset(stable_id_t id) const
  {
    ocudu_assert(contains(id), "Invalid stable_id");
    return index_map[id.value()];
  }

  template <typename... Args>
  stable_id_t emplace(Args&&... args)
  {
    static_assert((std::is_constructible_v<T, Args&&...>),
                  "Argument types must be constructible into corresponding vector::value_type.");
    unsigned offset    = index_reverse_map.size();
    unsigned chosen_id = free_list_head;
    if (chosen_id < index_map.size()) {
      free_list_head       = index_map[chosen_id];
      index_map[chosen_id] = offset;
    } else {
      ocudu_sanity_check(chosen_id == index_map.size(), "Invalid free_list_head");
      index_map.push_back(offset);
      free_list_head = chosen_id + 1;
    }
    index_reverse_map.emplace_back(chosen_id);
    objs.emplace_back(std::forward<Args>(args)...);
    return stable_id_t{chosen_id};
  }

  template <typename U>
  stable_id_t insert(U&& obj)
  {
    return emplace(std::forward<U>(obj));
  }

  bool erase(stable_id_t rem_id)
  {
    if (rem_id.value() >= index_map.size()) {
      return false;
    }
    const unsigned offset = index_map[rem_id.value()];
    if (offset >= index_reverse_map.size()) {
      return false;
    }
    ocudu_sanity_check(index_reverse_map[offset] == rem_id, "Invalid stable_id");

    if (offset != index_reverse_map.size() - 1) {
      std::swap(objs[offset], objs.back());
      index_reverse_map[offset]                    = index_reverse_map.back();
      index_map[index_reverse_map[offset].value()] = offset;
    }
    objs.pop_back();
    index_reverse_map.pop_back();
    index_map[rem_id.value()] = free_list_head;
    free_list_head            = rem_id.value();
    return true;
  }

  void erase(const_iterator it)
  {
    ocudu_assert(it.parent == this, "Iterator does not belong to this table");
    ocudu_assert(it.offset < size(), "Iterator out of range");
    erase(index_reverse_map[it.offset]);
  }

  void clear()
  {
    objs.clear();
    index_map.clear();
    index_reverse_map.clear();
    free_list_head = 0;
  }

  void reserve(unsigned size)
  {
    objs.reserve(size);
    index_map.reserve(size);
    index_reverse_map.reserve(size);
  }

  span<const T> unsorted() const { return objs; }
  span<T>       unsorted() { return objs; }

  /// Get iterator of elements pointing to the beginning. Note: this iterator is not sorted by stable_id.
  iterator       begin() { return iterator{*this, 0}; }
  const_iterator begin() const { return const_iterator{*this, 0}; }
  const_iterator cbegin() const { return const_iterator{*this, 0}; }
  sentinel       end() const { return {}; }
  sentinel       cend() const { return {}; }

private:
  /// List of objects in contiguous memory storage.
  std::vector<T> objs;
  /// This vector serves two purposes:
  /// - for stable_ids under use, it maps the stable_id (the index of index_map) to an index/offset in the objs vector
  /// - for stable_ids not under use, it serves as the next stable_id in an intrusive linked list of free stable_ids.
  std::vector<unsigned> index_map;
  /// Map from offset/index of the objs vector back to its respective stable_id.
  std::vector<stable_id_t> index_reverse_map;
  /// Intrusive linked list of free stable_ids (using holes of index_map as the node next stable_id element).
  unsigned free_list_head{0};
};

} // namespace ocudu
