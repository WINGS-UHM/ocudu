/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#pragma once

#include "../cell/resource_grid.h"
#include "../config/cell_configuration.h"
#include "ocudu/adt/bounded_bitset.h"
#include "ocudu/adt/circular_array.h"
#include "ocudu/adt/expected.h"
#include "ocudu/adt/slotted_array.h"
#include "ocudu/adt/span.h"
#include "ocudu/adt/static_vector.h"
#include "ocudu/ran/pucch/pucch_constants.h"
#include "ocudu/ran/slot_point.h"
#include <optional>

namespace ocudu {

namespace detail {

/// Represents the time-frequency grants of a PUCCH resource.
struct pucch_grants {
  /// Time-frequency grants of the first hop.
  grant_info first_hop;
  /// Time-frequency grant of the second hop (if intra-slot frequency hopping is enabled).
  std::optional<grant_info> second_hop;

  bool operator==(const pucch_grants& other) const
  {
    return first_hop == other.first_hop and second_hop == other.second_hop;
  }
  bool operator!=(const pucch_grants& other) const { return not(*this == other); }

  /// Checks if this pucch_grants overlaps with another pucch_grants.
  bool overlaps(const pucch_grants& other) const
  {
    // Check if the first grant overlaps with any of the other's grants.
    if (first_hop.overlaps(other.first_hop) or
        (other.second_hop.has_value() and first_hop.overlaps(*other.second_hop))) {
      return true;
    }
    // Check if the second grant (if any) overlaps with any of the other's grants.
    if (second_hop.has_value() and (second_hop->overlaps(other.first_hop) or
                                    (other.second_hop.has_value() and second_hop->overlaps(*other.second_hop)))) {
      return true;
    }
    return false;
  }
};

/// Represents the relevant information of a PUCCH resource for collision checking.
struct resource_info {
  /// PUCCH format of the resource.
  pucch_format format;
  /// Multiplexing index of the resource. Resources with different multiplexing indices are orthogonal and do not
  /// collide. It is computed from different parameters depending on the format:
  ///  - Format 0: initial cyclic shift.
  ///  - Format 1: initial cyclic shift, time domain OCC index.
  ///  - Format 2/3: not multiplexed (always 0).
  ///  - Format 4: OCC index.
  unsigned multiplexing_index;
  /// Time-frequency grants of the resource.
  pucch_grants grants;
};

/// \brief List of all PUCCH resources (common + dedicated) in the cell configuration.
using cell_resource_list = static_vector<resource_info, pucch_constants::MAX_NOF_TOT_CELL_RESOURCES>;

/// Compute the collision matrix for all PUCCH resources in the cell configuration.
cell_resource_list make_cell_resource_list(const cell_configuration& cell_cfg);

/// \brief Collision matrix indicating which resources collide with each other.
///  - C[i][j] = 1 if resource i collides with resource j, 0 otherwise.
using collision_matrix = static_vector<bounded_bitset<pucch_constants::MAX_NOF_TOT_CELL_RESOURCES>,
                                       pucch_constants::MAX_NOF_TOT_CELL_RESOURCES>;

/// Compute the collision matrix for all PUCCH resources in the cell configuration.
collision_matrix make_collision_matrix(const cell_resource_list& resources);

/// \brief Matrix of multiplexing regions indicating which resources can be multiplexed together.
///  - Each row represents a multiplexing region.
///  - M[i][j] = 1 if resource j belongs to multiplexing region i, 0 otherwise.
/// \remark Multiplexing regions with only one resource (i.e., non-multiplexed resources) are not represented in this
/// matrix. Therefore, we can have a maximum of $MAX_NOF_TOT_CELL_RESOURCES / 2$ rows.
using mux_regions_matrix = static_vector<bounded_bitset<pucch_constants::MAX_NOF_TOT_CELL_RESOURCES>,
                                         pucch_constants::MAX_NOF_TOT_CELL_RESOURCES / 2>;

/// Compute the multiplexing matrix for all PUCCH resources in the cell configuration.
mux_regions_matrix make_mux_regions_matrix(const cell_resource_list& resources);

} // namespace detail

/// \brief This class manages PUCCH resource collisions within a cell.
///
/// It keeps track of the usage of both common and dedicated resources for each slot, and provides methods to allocate
/// and free resources while accounting for collisions with other resources or other UL grants. Each operation updates
/// the provided UL resource grid accordingly.
///
/// PUCCH-to-UL grant collisions are handled via the UL resource grid, while PUCCH-to-PUCCH collisions are handled by
/// precomputing which resources collide with each other, taking into account the multiplexing capabilities of PUCCH.
///
/// \remark To deal with the multiplexing capabilities of some PUCCH formats, we define the following concepts:
/// - Multiplexing index. A number that identifies the orthogonal sequence used by a PUCCH resource. Computed from:
///   - Format 0: initial cyclic shift.
///   - Format 1: initial cyclic shift and time domain OCC index.
///   - Format 2/3: not multiplexed (always 0).
///   - Format 4: OCC index.
/// - Multiplexing region. A set of PUCCH resources that overlap in time and frequency but can be allocated together:
///   - They share the same time-frequency allocation.
///   - They share the same PUCCH format.
///   - They have different multiplexing indices.
class pucch_collision_manager
{
public:
  using r_pucch_t = bounded_integer<unsigned, 0, pucch_constants::MAX_NOF_CELL_COMMON_PUCCH_RESOURCES - 1>;

  pucch_collision_manager(const cell_configuration& cell_cfg);

  void slot_indication(slot_point sl_tx);
  void stop();

  /// Reasons for a PUCCH allocation failure.
  enum class alloc_failure_reason {
    PUCCH_COLLISION,
    UL_GRANT_COLLISION,
  };
  using alloc_result_t = error_type<alloc_failure_reason>;

  /// \brief Allocate a common PUCCH resource at a given slot.
  /// \return Success if the allocation was successful, otherwise an error indicating the reason of failure.
  alloc_result_t alloc_common(cell_slot_resource_grid& ul_res_grid, slot_point sl, r_pucch_t r_pucch);

  /// \brief Allocate a dedicated PUCCH resource at a given slot.
  /// \return Success if the allocation was successful, otherwise an error indicating the reason of failure.
  alloc_result_t alloc_ded(cell_slot_resource_grid& ul_res_grid, slot_point sl, unsigned cell_res_id);

  /// Free a common PUCCH resource at the given slot.
  /// \return True if the resource was successfully freed, false if the resource was not allocated.
  bool free_common(cell_slot_resource_grid& ul_res_grid, slot_point sl, r_pucch_t r_pucch);

  /// Free a dedicated PUCCH resource at the given slot.
  /// \return True if the resource was successfully freed, false if the resource was not allocated.
  bool free_ded(cell_slot_resource_grid& ul_res_grid, slot_point sl, unsigned cell_res_id);

private:
  using mux_region_lookup_t = slotted_array<size_t, pucch_constants::MAX_NOF_TOT_CELL_RESOURCES>;

  const cell_configuration& cell_cfg;
  /// List of all PUCCH resources in the cell.
  const detail::cell_resource_list resources;
  /// Precomputed collision matrix for all PUCCH resources.
  const detail::collision_matrix col_matrix;
  /// Precomputed multiplexing regions matrix for all PUCCH resources.
  const detail::mux_regions_matrix mux_matrix;
  /// Lookup table to get the multiplexing region of each resource.
  const mux_region_lookup_t mux_region_lookup;

  /// Allocation context for a specific slot.
  struct slot_context {
    /// Bitset representing the current usage state of all PUCCH resources (common and dedicated) in this slot.
    ///  - S[i] = 1 if resource i is in use, 0 otherwise.
    bounded_bitset<pucch_constants::MAX_NOF_TOT_CELL_RESOURCES> current_state;
    /// Resource grid that keeps track of the time-frequency grants of PUCCH resources in this slot.
    cell_slot_resource_grid pucch_res_grid;

    /// Default constructor needed by circular_array.
    slot_context() : pucch_res_grid({}) {}

    slot_context(const cell_configuration& cell_cfg) :
      current_state(pucch_constants::MAX_NOF_CELL_COMMON_PUCCH_RESOURCES + cell_cfg.ded_pucch_resources.size()),
      pucch_res_grid(cell_cfg.ul_cfg_common.freq_info_ul.scs_carrier_list)
    {
    }
  };

  // Ring buffer of slot contexts to keep track of PUCCH resource usage in recent slots.
  circular_array<slot_context, cell_resource_allocator::RING_ALLOCATOR_SIZE> slots_ctx;

  // Keeps track of the last slot_point used by the resource manager.
  slot_point last_sl_ind;

  /// Get the internal index of the dedicated PUCCH resource inside the collision manager.
  unsigned get_ded_idx(unsigned cell_res_id) const;

  /// Build the lookup table that maps each resource index to its multiplexing region.
  static mux_region_lookup_t build_mux_region_lookup(const detail::mux_regions_matrix& mux_matrix);

  /// \brief Allocate the PUCCH resource indexed by \ref res_idx at the given slot.
  /// \return Success if the allocation was successful, otherwise an error indicating the reason of failure.
  alloc_result_t alloc(cell_slot_resource_grid& ul_res_grid, slot_point sl, unsigned res_idx);

  /// \brief Free the PUCCH resource indexed by \c res_idx at the given slot.
  /// \return True if the resource was successfully freed, false if the resource was not allocated.
  bool free(cell_slot_resource_grid& ul_res_grid, slot_point sl, unsigned res_idx);
};

} // namespace ocudu
