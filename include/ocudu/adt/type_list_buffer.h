// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI

#pragma once

#include "ocudu/support/ocudu_assert.h"
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <new>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace ocudu {

namespace detail {

/// \brief Finds the zero-based index of type \c T in the type pack \c Types.
///
/// Uses explicit specialization for the "found" case so the not-found base case is never instantiated when T is
/// present (a ternary-based approach would eagerly instantiate both branches).
template <typename T, typename... Types>
struct type_list_index_helper;

/// T matches the head of the pack: index is 0.
template <typename T, typename... Rest>
struct type_list_index_helper<T, T, Rest...> {
  static constexpr size_t value = 0;
};

/// T does not match the head: advance by 1 and recurse into the tail.
template <typename T, typename First, typename... Rest>
struct type_list_index_helper<T, First, Rest...> {
  static constexpr size_t value = 1 + type_list_index_helper<T, Rest...>::value;
};

/// T was not found in the pack: trigger a compile error.
template <typename T>
struct type_list_index_helper<T> {
  template <typename>
  static constexpr bool always_false = false;
  static_assert(always_false<T>, "Type T is not present in the type_list_buffer's type list");
  static constexpr size_t value = 0;
};

/// \brief Checks whether \c T appears in the type pack \c Types.
template <typename T, typename... Types>
inline constexpr bool type_list_contains_v = (std::is_same_v<T, Types> || ...);

/// \brief Aligns \c val up to the nearest multiple of \c alignment (must be a power of two).
constexpr size_t align_up_v(size_t val, size_t alignment) noexcept
{
  return (val + alignment - 1) & ~(alignment - 1);
}

/// \brief Visits a single stored element given its type index and the buffer position just past the type index.
///
/// Dispatches at runtime using a fold expression over the compile-time index pack. Returns the buffer position of
/// the first byte after the visited element, or \c ~size_t{0} if the type index is out of range.
template <bool IsConst, typename... Types>
struct type_list_dispatch {
  using byte_ptr = std::conditional_t<IsConst, const uint8_t, uint8_t>;

  /// Invoke visitor on the element whose type index equals \p type_idx.
  /// \p after_idx is the buffer offset right after the type-index bytes.
  /// Returns the offset of the first byte after the element (start of the next record).
  template <typename Visitor, size_t... Is>
  static size_t
  run(size_t type_idx, size_t after_idx, byte_ptr* buf, Visitor& v, std::index_sequence<Is...> /*seq*/) noexcept(
      std::conjunction_v<std::is_nothrow_invocable<Visitor&, std::tuple_element_t<Is, std::tuple<Types...>>&>...>)
  {
    size_t next_pos = 0;
    // Fold over all indices; short-circuits once the matching index is found.
    bool found = ((type_idx == Is ? (next_pos = invoke<Is>(after_idx, buf, v), true) : false) || ...);
    return found ? next_pos : ~size_t{0};
  }

private:
  template <size_t I, typename Visitor>
  static size_t invoke(size_t after_idx, byte_ptr* buf, Visitor& v)
  {
    using T              = std::tuple_element_t<I, std::tuple<Types...>>;
    using obj_ref        = std::conditional_t<IsConst, const T&, T&>;
    const size_t obj_pos = align_up_v(after_idx, alignof(T));
    v(static_cast<obj_ref>(*std::launder(reinterpret_cast<std::conditional_t<IsConst, const T, T>*>(buf + obj_pos))));
    return obj_pos + sizeof(T);
  }
};

} // namespace detail

/// \brief Heterogeneous container that stores a sequence of objects of a fixed type list in a packed memory buffer.
///
/// Unlike a \c std::vector<std::variant<Types...>>, each stored element occupies exactly
/// \c sizeof(type_index) + \c sizeof(T) bytes (plus at most \c alignof(T)-1 alignment-padding bytes), so smaller
/// types take up less space.
///
/// Layout of each record in the backing buffer:
/// \code
///   [ type_idx_t  idx ][ 0..N padding bytes ][ T  object ]
///                       <---alignof(T)------->
/// \endcode
///
/// Iterating the buffer reads the type index at the start of each record to determine the stored type and the size
/// needed to advance to the next record.
///
/// \tparam Types List of types that may be stored. All types must be destructible.
template <typename... Types>
class type_list_buffer
{
  static_assert(sizeof...(Types) > 0, "type_list_buffer requires at least one type");
  static_assert(sizeof...(Types) <= static_cast<size_t>(std::numeric_limits<uint16_t>::max()),
                "type_list_buffer: too many types (max 65535)");

  // Use the smallest unsigned integer type that can represent all type indices.
  using type_idx_t = std::
      conditional_t<(sizeof...(Types) <= static_cast<size_t>(std::numeric_limits<uint8_t>::max())), uint8_t, uint16_t>;

  template <typename T>
  static constexpr type_idx_t type_index_of_v =
      static_cast<type_idx_t>(detail::type_list_index_helper<T, Types...>::value);

  using idx_seq        = std::make_index_sequence<sizeof...(Types)>;
  using const_dispatch = detail::type_list_dispatch<true, Types...>;
  using mut_dispatch   = detail::type_list_dispatch<false, Types...>;

  std::vector<uint8_t> buf;
  size_t               nof_elements = 0;

  /// Calls the destructor of every stored object (no-op if all types are trivially destructible).
  void destroy_all() noexcept
  {
    if constexpr (!std::conjunction_v<std::is_trivially_destructible<Types>...>) {
      for_each([](auto& obj) {
        using T = std::decay_t<decltype(obj)>;
        obj.~T();
      });
    }
  }

public:
  type_list_buffer() = default;
  ~type_list_buffer() { destroy_all(); }

  type_list_buffer(const type_list_buffer& other)
  {
    if constexpr (std::conjunction_v<std::is_trivially_copyable<Types>...>) {
      buf          = other.buf;
      nof_elements = other.nof_elements;
    } else {
      buf.reserve(other.buf.size());
      other.for_each([this](const auto& obj) { push(obj); });
    }
  }

  type_list_buffer& operator=(const type_list_buffer& other)
  {
    if (this != &other) {
      clear();
      if constexpr (std::conjunction_v<std::is_trivially_copyable<Types>...>) {
        buf          = other.buf;
        nof_elements = other.nof_elements;
      } else {
        buf.reserve(other.buf.size());
        other.for_each([this](const auto& obj) { push(obj); });
      }
    }
    return *this;
  }

  type_list_buffer(type_list_buffer&&) noexcept            = default;
  type_list_buffer& operator=(type_list_buffer&&) noexcept = default;

  /// Returns the number of stored elements.
  size_t size() const noexcept { return nof_elements; }

  /// Returns \c true if the buffer holds no elements.
  bool empty() const noexcept { return nof_elements == 0; }

  /// Returns the number of bytes currently used by the backing buffer (includes type-index and padding bytes).
  size_t byte_size() const noexcept { return buf.size(); }

  /// Hint the backing buffer to reserve at least \p bytes of storage.
  void reserve(size_t bytes) { buf.reserve(bytes); }

  /// Appends a copy or move of \p val to the buffer.
  /// \tparam T must appear in \c Types.
  template <typename T>
  void push(T&& val)
  {
    using U = std::decay_t<T>;
    static_assert(detail::type_list_contains_v<U, Types...>,
                  "push(): T is not present in this type_list_buffer's type list");
    emplace<U>(std::forward<T>(val));
  }

  /// Constructs a \c T in-place at the end of the buffer.
  /// \tparam T must appear in \c Types.
  template <typename T, typename... Args>
  void emplace(Args&&... args)
  {
    static_assert(detail::type_list_contains_v<T, Types...>,
                  "emplace(): T is not present in this type_list_buffer's type list");

    constexpr type_idx_t idx     = type_index_of_v<T>;
    const size_t         start   = buf.size();
    const size_t         obj_pos = detail::align_up_v(start + sizeof(type_idx_t), alignof(T));

    buf.resize(obj_pos + sizeof(T));

    std::memcpy(buf.data() + start, &idx, sizeof(type_idx_t));

    // Zero-fill alignment-padding bytes so the buffer has no uninitialized reads.
    if (const size_t pad = obj_pos - start - sizeof(type_idx_t); pad > 0) {
      std::memset(buf.data() + start + sizeof(type_idx_t), 0, pad);
    }

    new (buf.data() + obj_pos) T(std::forward<Args>(args)...);
    ++nof_elements;
  }

  /// Calls \p v once for every stored element in insertion order (const overload).
  /// \p v must be callable with each type in \c Types (e.g. a generic lambda or a struct with overloads).
  template <typename Visitor>
  void for_each(Visitor v) const
  {
    size_t pos = 0;
    while (pos < buf.size()) {
      type_idx_t idx;
      std::memcpy(&idx, buf.data() + pos, sizeof(type_idx_t));
      const size_t after_idx = pos + sizeof(type_idx_t);

      const size_t next = const_dispatch::run(static_cast<size_t>(idx), after_idx, buf.data(), v, idx_seq{});
      ocudu_assert(next != ~size_t{0}, "Corrupt type_list_buffer: invalid type index {}", static_cast<unsigned>(idx));
      pos = next;
    }
  }

  /// Calls \p v once for every stored element in insertion order (mutable overload).
  template <typename Visitor>
  void for_each(Visitor v)
  {
    size_t pos = 0;
    while (pos < buf.size()) {
      type_idx_t idx;
      std::memcpy(&idx, buf.data() + pos, sizeof(type_idx_t));
      const size_t after_idx = pos + sizeof(type_idx_t);

      const size_t next = mut_dispatch::run(static_cast<size_t>(idx), after_idx, buf.data(), v, idx_seq{});
      ocudu_assert(next != ~size_t{0}, "Corrupt type_list_buffer: invalid type index {}", static_cast<unsigned>(idx));
      pos = next;
    }
  }

  /// Destroys all stored elements and resets the buffer to an empty state.
  void clear() noexcept
  {
    destroy_all();
    buf.clear();
    nof_elements = 0;
  }
};

} // namespace ocudu
