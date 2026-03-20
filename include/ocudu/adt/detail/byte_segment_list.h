// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI

#pragma once

#include "ocudu/adt/detail/byte_buffer_memory_resource.h"
#include "ocudu/adt/detail/intrusive_ptr.h"
#include "ocudu/adt/expected.h"
#include "ocudu/adt/intrusive_list.h"
#include "ocudu/adt/span.h"
#include "ocudu/support/memory_pool/linear_memory_allocator.h"
#include <cstdlib>
#include <iterator>

namespace ocudu::detail {

/// \brief Intrusive singly-linked list node carrying a raw memory payload.
///
/// The payload span covers the user-accessible bytes of the pool allocation belonging to this node.
/// For the node co-located with the control block, the payload starts after the control block;
/// for all other nodes it starts immediately after the node header.
struct byte_segment_node : intrusive_forward_list_element<> {
  span<uint8_t> payload;
};

/// Non-owning, intrusive singly-linked list of \c byte_segment_node objects.
using byte_segment_list = intrusive_forward_list<byte_segment_node>;

/// \brief Ref-counted, owning list of raw memory segments backed by a \c byte_buffer_memory_resource.
///
/// On the first append_segment() call the control block and the first user segment are co-located
/// in a single pool allocation. The node's payload span is adjusted to start after the control
/// block so every requested byte is still accessible.
///
/// Copyable (shallow, ref-counted) and movable.
class shared_byte_segment_list
{
  struct control_block {
    /// Intrusive linked-list of byte segments (including the cb_node as the first element).
    byte_segment_list segments;
    /// Reference count for this control block.
    intrusive_ptr_atomic_ref_counter ref_count;
    /// Pool used to allocate/deallocate segments.
    byte_buffer_memory_resource* pool = nullptr;

    ~control_block() = default;

    /// Returns the segment node that is co-located with this control block.
    /// Layout is [ node | control_block ], so the node is sizeof(byte_segment_node) before this.
    byte_segment_node* node_in_same_block() const
    {
      return reinterpret_cast<byte_segment_node*>(reinterpret_cast<char*>(const_cast<control_block*>(this)) -
                                                  sizeof(byte_segment_node));
    }

    void destroy_cb()
    {
      byte_buffer_memory_resource* p       = pool;
      byte_segment_node*           cb_node = node_in_same_block();

      // Free all segment nodes except the CB node (it shares a pool block with the CB itself).
      byte_segment_node* node = segments.empty() ? nullptr : &segments.front();
      while (node != nullptr) {
        byte_segment_node* next = static_cast<byte_segment_node*>(node->next_node);
        if (node != cb_node) {
          node->~byte_segment_node();
          p->deallocate(node);
        }
        node = next;
      }

      // Destroy the control block in-place, then release the pool block that holds it.
      this->~control_block();
      cb_node->~byte_segment_node();
      p->deallocate(cb_node);
    }

  private:
    friend void intrusive_ptr_inc_ref(control_block* p) { p->ref_count.inc_ref(); }
    friend void intrusive_ptr_dec_ref(control_block* p)
    {
      if (p->ref_count.dec_ref()) {
        p->destroy_cb();
      }
    }
    friend bool intrusive_ptr_is_unique(control_block* p) { return p->ref_count.unique(); }
  };

  /// Malloc-backed memory resource used when no pool is provided.
  struct heap_byte_buffer_memory_resource final : public ocudu::byte_buffer_memory_resource {
  private:
    span<uint8_t> do_allocate(size_t size_hint, size_t /*alignment*/) override
    {
      void* p = std::malloc(size_hint);
      if (p == nullptr) {
        return {};
      }
      return {static_cast<uint8_t*>(p), size_hint};
    }

    void do_deallocate(void* p) override { std::free(p); }

    bool do_is_equal(const byte_buffer_memory_resource& other) const noexcept override { return this == &other; }
  };

  /// Heap-based memory pool.
  inline static heap_byte_buffer_memory_resource heap_pool;

  template <bool IsConst>
  struct iter_impl {
    using node_ptr  = std::conditional_t<IsConst, const byte_segment_node*, byte_segment_node*>;
    using reference = std::conditional_t<IsConst, span<const uint8_t>, span<uint8_t>>;

    using iterator_category = std::forward_iterator_tag;
    using value_type        = reference;
    using difference_type   = std::ptrdiff_t;
    using pointer           = void;

    node_ptr current;

    reference  operator*() const { return current->payload; }
    iter_impl& operator++()
    {
      current = static_cast<node_ptr>(current->next_node);
      return *this;
    }
    bool operator==(const iter_impl& other) const { return current == other.current; }
    bool operator!=(const iter_impl& other) const { return current != other.current; }
  };

public:
  using iterator       = iter_impl<false>;
  using const_iterator = iter_impl<true>;

  /// Creates a segment list that uses a malloc-backed pool on first append_segment() call.
  shared_byte_segment_list() noexcept = default;

  /// Create a shared_byte_segment_list with the specified pool and the provided segment size hint.
  static expected<shared_byte_segment_list> make(size_t seg_size_hint, byte_buffer_memory_resource& p) noexcept
  {
    expected<shared_byte_segment_list> result;
    result.emplace();
    if (OCUDU_UNLIKELY(not result->init_ctrl_block(seg_size_hint, p))) {
      result->clear();
    }
    return result;
  }

  /// Returns true if no segments have been appended.
  bool empty() const noexcept { return ctrl_blk_ptr == nullptr || ctrl_blk_ptr->segments.empty(); }

  iterator begin() noexcept { return {empty() ? nullptr : &ctrl_blk_ptr->segments.front()}; }
  iterator end() noexcept { return {nullptr}; }

  const_iterator begin() const noexcept { return {empty() ? nullptr : &ctrl_blk_ptr->segments.front()}; }
  const_iterator end() const noexcept { return {nullptr}; }

  /// Total number of payload bytes across all segments (O(n)).
  size_t length() const noexcept
  {
    size_t total = 0;
    for (span<const uint8_t> seg : *this) {
      total += seg.size();
    }
    return total;
  }

  /// Allocates a new segment and appends it to the list.
  /// \param size_hint Desired payload size; actual size depends on the pool block size.
  /// \returns True on success, false on allocation failure.
  [[nodiscard]] bool append_segment(size_t size_hint)
  {
    if (ctrl_blk_ptr == nullptr) {
      if (OCUDU_UNLIKELY(not init_ctrl_block(size_hint + sizeof(control_block), heap_pool))) {
        return false;
      }
      return true;
    }
    byte_segment_node* node = create_segment(size_hint);
    if (node == nullptr) {
      return false;
    }
    ctrl_blk_ptr->segments.push_back(node);
    return true;
  }

  /// Payload span of the first segment, or empty span if empty().
  span<uint8_t>       front() noexcept { return empty() ? span<uint8_t>{} : ctrl_blk_ptr->segments.front().payload; }
  span<const uint8_t> front() const noexcept
  {
    return empty() ? span<const uint8_t>{} : ctrl_blk_ptr->segments.front().payload;
  }

  /// Payload span of the last segment, or empty span if empty().
  span<uint8_t>       tail() noexcept { return empty() ? span<uint8_t>{} : ctrl_blk_ptr->segments.back().payload; }
  span<const uint8_t> tail() const noexcept
  {
    return empty() ? span<const uint8_t>{} : ctrl_blk_ptr->segments.back().payload;
  }

  /// Releases the control block (decrements ref count; frees all nodes when it reaches zero).
  void clear() { ctrl_blk_ptr.reset(); }

private:
  /// Allocates the first segment, co-locating the control block inside its pool block.
  /// The node's payload is set to the bytes that follow the control block.
  [[nodiscard]] bool init_ctrl_block(size_t size_hint, byte_buffer_memory_resource& p)
  {
    span<uint8_t> mem_block = p.allocate(sizeof(byte_segment_node) + sizeof(control_block) + size_hint);
    if (mem_block.size() < sizeof(byte_segment_node) + sizeof(control_block)) {
      return false;
    }

    linear_memory_allocator arena{mem_block.data(), mem_block.size()};

    // Allocate byte_segment_list header.
    void* node_region = arena.allocate(sizeof(byte_segment_node), alignof(byte_segment_node));
    auto* node        = new (node_region) byte_segment_node{};

    // Allocate control block.
    void* cb_region    = arena.allocate(sizeof(control_block), alignof(control_block));
    ctrl_blk_ptr       = new (cb_region) control_block{};
    ctrl_blk_ptr->pool = &p;

    // Payload starts after the control block.
    node->payload = {reinterpret_cast<uint8_t*>(ctrl_blk_ptr.get() + 1), arena.nof_bytes_left()};
    ctrl_blk_ptr->segments.push_back(node);
    return true;
  }

  /// Allocates a new segment from the pool stored in the control block.
  [[nodiscard]] byte_segment_node* create_segment(size_t size_hint)
  {
    span<uint8_t> mem_block = ctrl_blk_ptr->pool->allocate(sizeof(byte_segment_node) + size_hint);
    if (mem_block.size() < sizeof(byte_segment_node) + 1) {
      return nullptr;
    }

    linear_memory_allocator arena{mem_block.data(), mem_block.size()};

    /// Allocate segment header in place.
    void* node_region = arena.allocate(sizeof(byte_segment_node), alignof(byte_segment_node));
    auto* node        = new (node_region) byte_segment_node{};
    node->payload     = {reinterpret_cast<uint8_t*>(node + 1), arena.nof_bytes_left()};
    return node;
  }

  /// Pointer to control block where the linked-list of segments is stored.
  intrusive_ptr<control_block> ctrl_blk_ptr;
};

} // namespace ocudu::detail
