// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#pragma once

#include "ocudu/adt/circular_array.h"
#include "ocudu/ran/slot_point.h"
#include <mutex>
#include <utility>

namespace ocudu {

/// \brief Thread-safe resource request pool.
///
/// Contains a pool of requests. A request consist of a slot point and a templated resource. Requests are indexed by
/// slots. The pool access is thread-safe and it is done by exchange requests.
///
/// \tparam ResourceType     Resource type.
/// \tparam requestArraySize Maximum number of requests contained in the array.
template <typename ResourceType, unsigned requestArraySize = 16>
class resource_request_pool
{
public:
  /// Get the maximum number of requests that can be held by the array.
  static constexpr unsigned get_request_array_size() { return requestArraySize; }

  /// Internal storage type.
  struct request_type {
    slot_point   slot;
    ResourceType resource;
  };

  /// \brief Exchanges a request from the pool by the given one.
  /// \param[in] request The given request, it is copied into <tt>request.slot.system_slot() % requestArraySize</tt>.
  /// \return The previous request at position <tt> request.slot.system_slot() % requestArraySize </tt>.
  request_type exchange(request_type request)
  {
    return requests[request.slot.system_slot()].exchange(std::move(request));
  }

private:
  /// Wraps the request in a thread-safe access.
  class request_wrapper
  {
  public:
    /// Default constructor - constructs an empty request.
    request_wrapper() : request({slot_point(), ResourceType()}) {}

    /// \brief Exchanges the previous request with a new request.
    /// \param[in] new_request New request.
    /// \return A copy of the previous request.
    request_type exchange(request_type new_request)
    {
      std::unique_lock<std::mutex> lock(mutex);
      request_type                 old_request = std::move(request);
      request                                  = std::move(new_request);
      return old_request;
    }

  private:
    request_type request;
    std::mutex   mutex;
  };

  /// Request storage, indexed by slots.
  circular_array<request_wrapper, requestArraySize> requests;
};

} // namespace ocudu
