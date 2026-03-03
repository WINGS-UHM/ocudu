// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#pragma once
#include "xnap_ue_logger.h"
#include "ocudu/xnap/xnap_types.h"

namespace ocudu::ocucp {

struct xnap_ue_ids {
  ue_index_t   ue_index   = ue_index_t::invalid;
  xnap_ue_id_t xnap_ue_id = xnap_ue_id_t::invalid;
};

struct xnap_ue_context {
  xnap_ue_ids    ue_ids;
  xnap_ue_logger logger;

  xnap_ue_context(ue_index_t ue_index_, xnap_ue_id_t xnap_ue_id_) :
    ue_ids({ue_index_, xnap_ue_id_}), logger("XNAP", {ue_index_, xnap_ue_id_})
  {
  }
};

class xnap_ue_context_list
{
public:
  xnap_ue_context_list(ocudulog::basic_logger& logger_) : logger(logger_) {}

  /// \brief Checks whether a UE with the given XNAP UE ID exists.
  /// \param[in] xnap_ue_id The XNAP UE ID used to find the UE.
  /// \return True when a UE for the given XNAP UE ID exists, false otherwise.
  bool contains(xnap_ue_id_t xnap_ue_id) const { return ues.find(xnap_ue_id) != ues.end(); }

  /// \brief Checks whether a UE with the given UE index exists.
  /// \param[in] ue_index The UE index used to find the UE.
  /// \return True when a UE for the given UE index exists, false otherwise.
  bool contains(ue_index_t ue_index) const
  {
    if (ue_index_to_xnap_ue_id.find(ue_index) == ue_index_to_xnap_ue_id.end()) {
      return false;
    }
    if (ues.find(ue_index_to_xnap_ue_id.at(ue_index)) == ues.end()) {
      return false;
    }
    return true;
  }

  xnap_ue_context& operator[](xnap_ue_id_t xnap_ue_id)
  {
    ocudu_assert(
        ues.find(xnap_ue_id) != ues.end(), "xnap_ue={}: XNAP UE context not found", fmt::underlying(xnap_ue_id));
    return ues.at(xnap_ue_id);
  }

  xnap_ue_context& operator[](ue_index_t ue_index)
  {
    ocudu_assert(
        ue_index_to_xnap_ue_id.find(ue_index) != ue_index_to_xnap_ue_id.end(), "ue={}: XNAP UE ID not found", ue_index);
    ocudu_assert(ues.find(ue_index_to_xnap_ue_id.at(ue_index)) != ues.end(),
                 "xnap_ue={}: XNAP UE context not found",
                 fmt::underlying(ue_index_to_xnap_ue_id.at(ue_index)));
    return ues.at(ue_index_to_xnap_ue_id.at(ue_index));
  }

  xnap_ue_context* find(xnap_ue_id_t xnap_ue_id)
  {
    auto it = ues.find(xnap_ue_id);
    if (it == ues.end()) {
      return nullptr;
    }
    return &it->second;
  }

  const xnap_ue_context* find(xnap_ue_id_t xnap_ue_id) const
  {
    auto it = ues.find(xnap_ue_id);
    if (it == ues.end()) {
      return nullptr;
    }
    return &it->second;
  }

  xnap_ue_context& add_ue(ue_index_t ue_index, xnap_ue_id_t xnap_ue_id)
  {
    ocudu_assert(ue_index != ue_index_t::invalid, "Invalid ue_index={}", fmt::underlying(ue_index));
    ocudu_assert(xnap_ue_id != xnap_ue_id_t::invalid, "Invalid xnap_ue_id={}", fmt::underlying(xnap_ue_id));

    logger.debug("ue={} xnap_ue={}: XNAP UE context created", fmt::underlying(ue_index), fmt::underlying(xnap_ue_id));
    ues.emplace(
        std::piecewise_construct, std::forward_as_tuple(xnap_ue_id), std::forward_as_tuple(ue_index, xnap_ue_id));
    ue_index_to_xnap_ue_id.emplace(ue_index, xnap_ue_id);
    return ues.at(xnap_ue_id);
  }

  void update_ue_index(ue_index_t new_ue_index, ue_index_t old_ue_index)
  {
    ocudu_assert(new_ue_index != ue_index_t::invalid, "Invalid new_ue_index={}", new_ue_index);
    ocudu_assert(old_ue_index != ue_index_t::invalid, "Invalid old_ue_index={}", old_ue_index);
    ocudu_assert(ue_index_to_xnap_ue_id.find(old_ue_index) != ue_index_to_xnap_ue_id.end(),
                 "ue={}: XNAP-UE-ID not found",
                 old_ue_index);

    xnap_ue_id_t xnap_ue_id = ue_index_to_xnap_ue_id.at(old_ue_index);

    ocudu_assert(
        ues.find(xnap_ue_id) != ues.end(), "xnap_ue={}: XNAP UE context not found", fmt::underlying(xnap_ue_id));

    // Update UE context.
    ues.at(xnap_ue_id).ue_ids.ue_index = new_ue_index;

    // Update lookups.
    ue_index_to_xnap_ue_id.emplace(new_ue_index, xnap_ue_id);
    ue_index_to_xnap_ue_id.erase(old_ue_index);

    ues.at(xnap_ue_id).logger.set_prefix({ues.at(xnap_ue_id).ue_ids.ue_index, xnap_ue_id});

    ues.at(xnap_ue_id).logger.log_debug("Updated UE index from ue_index={}", old_ue_index);
  }

  void remove_ue_context(ue_index_t ue_index)
  {
    ocudu_assert(ue_index != ue_index_t::invalid, "Invalid ue_index={}", ue_index);

    if (ue_index_to_xnap_ue_id.find(ue_index) == ue_index_to_xnap_ue_id.end()) {
      logger.warning("ue={}: XNAP-UE-ID not found", ue_index);
      return;
    }

    // Remove UE from lookup.
    xnap_ue_id_t xnap_ue_id = ue_index_to_xnap_ue_id.at(ue_index);
    ue_index_to_xnap_ue_id.erase(ue_index);

    if (ues.find(xnap_ue_id) == ues.end()) {
      logger.warning("xnap_ue={}: XNAP UE context not found", fmt::underlying(xnap_ue_id));
      return;
    }

    ues.at(xnap_ue_id).logger.log_debug("Removing XNAP UE context");

    ues.erase(xnap_ue_id);
  }

  size_t size() const { return ues.size(); }

  /// \brief Get the next available XNAP-UE-ID.
  xnap_ue_id_t allocate_xnap_ue_id()
  {
    // Return invalid when no XNAP-UE-ID is available.
    if (ue_index_to_xnap_ue_id.size() == MAX_NOF_XNAP_UES) {
      return xnap_ue_id_t::invalid;
    }

    // Check if the next_xnap_ue_id is available.
    if (ues.find(next_xnap_ue_id) == ues.end()) {
      xnap_ue_id_t ret = next_xnap_ue_id;
      // Increase the next XNAP-UE-ID.
      increase_next_xnap_ue_id();
      return ret;
    }

    // Iterate over all ids starting with the next_xnap_ue_id to find the available id.
    while (true) {
      // Iterate over ue_index_to_xnap_ue_id.
      auto it = std::find_if(ue_index_to_xnap_ue_id.begin(), ue_index_to_xnap_ue_id.end(), [this](auto& u) {
        return u.second == next_xnap_ue_id;
      });

      // Return the ID if it is not already used.
      if (it == ue_index_to_xnap_ue_id.end()) {
        xnap_ue_id_t ret = next_xnap_ue_id;
        // Increase the next XNAP-UE-ID.
        increase_next_xnap_ue_id();
        return ret;
      }

      // Increase the next XNAP-UE-ID and try again.
      increase_next_xnap_ue_id();
    }

    return xnap_ue_id_t::invalid;
  }

protected:
  xnap_ue_id_t next_xnap_ue_id = xnap_ue_id_t::min;

private:
  ocudulog::basic_logger& logger;

  inline void increase_next_xnap_ue_id()
  {
    if (next_xnap_ue_id == xnap_ue_id_t::max) {
      // Reset XNAP-UE-ID counter.
      next_xnap_ue_id = xnap_ue_id_t::min;
    } else {
      // Increase XNAP-UE-ID counter.
      next_xnap_ue_id = uint_to_xnap_ue_id(xnap_ue_id_to_uint(next_xnap_ue_id) + 1);
    }
  }

  // Note: Given that UEs will self-remove from the map, we don't want to destructor to clear the lookups beforehand.
  std::unordered_map<ue_index_t, xnap_ue_id_t>      ue_index_to_xnap_ue_id; // indexed by ue_index
  std::unordered_map<xnap_ue_id_t, xnap_ue_context> ues;                    // indexed by xnap_ue_id_t
};

} // namespace ocudu::ocucp
