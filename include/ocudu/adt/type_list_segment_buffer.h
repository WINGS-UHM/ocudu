// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI

#pragma once

#include "ocudu/adt/byte_buffer.h"
#include "ocudu/adt/type_list_buffer.h"

namespace ocudu {

/// \brief Heterogeneous container that stores a sequence of objects of a fixed type list backed by a \c byte_buffer.
///
/// The buffer is organized as a linked list of \c byte_buffer segments. At the start of each segment a
/// \c type_list_buffer_view is placement-constructed; the rest of the segment's payload is the view's backing store.
/// Records are appended through the tail view. When it is full a fresh segment is allocated, a new view is
/// placement-constructed there, and the record goes into that segment instead.
///
/// \note For types with non-trivial destructors, a shallow copy must not outlive the copy that ultimately destroys
///       the buffer, since the destructor walks every segment and calls each view's destructor (which destroys the
///       stored objects in the shared memory).
///
/// \tparam Types List of types that may be stored. All types must be destructible.
template <typename... Types>
class type_list_segment_buffer
{
  using view_t = type_list_buffer_view<Types...>;

public:
  type_list_segment_buffer() = default;

  ~type_list_segment_buffer() { destroy_all(); }

  type_list_segment_buffer(const type_list_segment_buffer& other) noexcept : buf(other.buf.copy()) {}
  type_list_segment_buffer& operator=(const type_list_segment_buffer& other) noexcept
  {
    if (this != &other) {
      buf = other.buf.copy();
    }
    return *this;
  }

  type_list_segment_buffer(type_list_segment_buffer&& other) noexcept = default;

  type_list_segment_buffer& operator=(type_list_segment_buffer&& other) noexcept
  {
    if (this != &other) {
      destroy_all();
      buf = std::move(other.buf);
    }
    return *this;
  }

  /// Returns the total number of stored elements (O(number of segments)).
  size_t size() const noexcept
  {
    size_t total = 0;
    for (auto seg : buf.segments()) {
      total += seg_to_view(seg)->size();
    }
    return total;
  }

  bool empty() const noexcept { return buf.empty(); }

  /// Returns the total number of record bytes used across all segments.
  size_t byte_size() const noexcept
  {
    size_t total = 0;
    for (auto seg : buf.segments()) {
      total += seg_to_view(seg)->byte_size();
    }
    return total;
  }

  /// Calls \p visitor once for every stored element in insertion order (const overload).
  template <typename Visitor>
  void for_each(Visitor&& visitor) const
  {
    for (auto seg : buf.segments()) {
      seg_to_view(seg)->for_each(visitor);
    }
  }

  /// Calls \p visitor once for every stored element in insertion order (mutable overload).
  template <typename Visitor>
  void for_each(Visitor&& visitor)
  {
    for (auto seg : buf.segments()) {
      seg_to_view(seg)->for_each(visitor);
    }
  }

  /// Destroys all stored elements and resets the buffer to an empty state.
  void clear() noexcept
  {
    destroy_all();
    buf.clear();
  }

  /// Appends a copy or move of \p val to the buffer.
  /// \returns A pointer to the newly stored element, or \c nullptr on allocation failure.
  template <typename T>
  std::decay_t<T>* push(T&& val)
  {
    using U = std::decay_t<T>;
    static_assert(type_list_helper::contains_v<U, Types...>,
                  "push(): T is not present in this type_list_segment_buffer's type list");
    return emplace<U>(std::forward<T>(val));
  }

  /// Constructs a \c T in-place at the end of the buffer.
  /// \returns A pointer to the newly constructed element, or \c nullptr on allocation failure.
  template <typename T, typename... Args>
  T* emplace(Args&&... args)
  {
    static_assert(type_list_helper::contains_v<T, Types...>,
                  "emplace(): T is not present in this type_list_segment_buffer's type list");
    if (buf.empty() && !alloc_new_segment()) {
      return nullptr;
    }
    T* obj = tail_view()->template emplace<T>(std::forward<Args>(args)...);
    if (obj != nullptr) {
      return obj;
    }
    // Current segment is full; allocate a new one and retry.
    if (!alloc_new_segment()) {
      return nullptr;
    }
    return tail_view()->template emplace<T>(std::forward<Args>(args)...);
  }

private:
  static view_t* seg_to_view(span<uint8_t> seg) noexcept { return std::launder(reinterpret_cast<view_t*>(seg.data())); }

  static const view_t* seg_to_view(span<const uint8_t> seg) noexcept
  {
    return std::launder(reinterpret_cast<const view_t*>(seg.data()));
  }

  /// Returns a pointer to the view in the tail segment. Requires \c !buf.empty().
  view_t* tail_view() noexcept { return seg_to_view(buf.tail_segment()); }

  /// Allocates a fresh segment, placement-constructs a \c view_t at its start, and appends it to \c buf.
  bool alloc_new_segment() noexcept
  {
    // Extend by 1 byte to ensure a new segment is allocated (tailroom is always 0 here:
    // either buf is empty, or the tail view is full and we filled the segment completely).
    if (!buf.resize(buf.length() + 1)) {
      return false;
    }
    // Fill the rest of the new tail segment so the view gets the maximum backing store.
    const size_t tail_free = buf.tailroom_capacity();
    if (tail_free > 0 && !buf.resize(buf.length() + tail_free)) {
      return false;
    }
    span<uint8_t> new_seg = buf.tail_segment();
    new (new_seg.data()) view_t(span<uint8_t>{new_seg.data() + sizeof(view_t), new_seg.size() - sizeof(view_t)});
    return true;
  }

  /// Calls the destructor of every \c view_t (which in turn destroys all stored objects).
  void destroy_all() noexcept
  {
    for (auto seg : buf.segments()) {
      seg_to_view(seg)->~view_t();
    }
  }

  byte_buffer buf;
};

} // namespace ocudu
