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

#include "srs_resource_generator.h"
#include "ocudu/du/du_cell_config.h"
#include "ocudu/ran/srs/srs_bandwidth_configuration.h"

namespace ocudu {
namespace odu {

struct cell_group_config;

// Maximum number of SRS resources that can be generated in a cell.
// [Implementation-defined] We assume each UE has one and only one resource.
static constexpr unsigned MAX_NOF_CELL_SRS_RES = MAX_NOF_DU_UES;

/// This abstract class defines the methods that the DU SRS resource manager must implement. The implementation of this
/// class defines different policies for the SRS allocation.
class du_srs_resource_manager
{
public:
  virtual ~du_srs_resource_manager() = default;

  /// \brief Allocate SRS resources for a given UE. The resources are stored in the UE's cell group config.
  /// The function allocates the UE the resources from a common pool.
  /// \return true if allocation was successful.
  virtual bool alloc_resources(cell_group_config& cell_grp_cfg) = 0;

  /// \brief Deallocate the SRS resources for a given UE and return the used resource to the common pool.
  virtual void dealloc_resources(cell_group_config& cell_grp_cfg) = 0;

  /// Gets the current number of free SRS resource ID and offset pairs.
  virtual unsigned get_nof_srs_free_res_offsets(du_cell_index_t cell_idx) const = 0;
};

} // namespace odu
} // namespace ocudu
