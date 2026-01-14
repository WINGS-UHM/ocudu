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

#include "du_srs_resource_manager.h"
#include "srs_resource_generator.h"
#include "ocudu/du/du_cell_config.h"
#include "ocudu/ran/srs/srs_bandwidth_configuration.h"

namespace ocudu {
namespace odu {

struct cell_group_config;

/// This class implements the MAX UL throughput policy for the SRS allocation. The SRS resources are allocated with the
/// objective to minimize the number of slots and symbols that contain the SRS resources, to reduce as much as possible
/// the slots and symbols resources taken from the PUSCH. The drawback of this policy is that it can increase the
/// inter-slot SRS interference among different UEs.
class du_srs_aperiodic_res_mng : public du_srs_resource_manager
{
public:
  explicit du_srs_aperiodic_res_mng(span<const du_cell_config> cell_cfg_list_);

  /// The SRS resources are allocate according to the following policy:
  /// - Give priority to resources that are placed on partially-UL slots, first.
  /// - Then, give priority to SRS resources that is placed on the symbol interval (i.e, the symbols interval used by an
  /// SRS resource) closest to the end of the slot.
  /// - If a symbol interval on a particular slot is already used and not completely full, then give priority to this
  /// symbol interval over any other symbol intervals on the same or on different slots.
  bool alloc_resources(cell_group_config& cell_grp_cfg) override;

  void dealloc_resources(cell_group_config& cell_grp_cfg) override;

  unsigned get_nof_srs_free_res_offsets(du_cell_index_t cell_idx) const override
  {
    // Return max value, as there is no hard limitation in case of aperiodic SRS.
    return cell_context::max_nof_srs_res;
  }

private:
  struct cell_context {
    cell_context(const du_cell_config& cfg);

    // Returns the DU SRS resource with the given cell resource ID from the cell list of resources.
    std::vector<du_srs_resource>::const_iterator get_du_srs_res_cfg(unsigned cell_res_id)
    {
      if (cell_res_id >= cell_srs_res_list.size() or cell_srs_res_list[cell_res_id].cell_res_id != cell_res_id) {
        ocudu_assertion_failure("Cell resource ID out of range or invalid");
        return cell_srs_res_list.end();
      }
      return cell_srs_res_list.cbegin() + cell_res_id;
    }

    void fill_srs_res_parameters(srs_config::srs_resource& res_out, const du_srs_resource& res_in) const;

    using srs_set_t = static_vector<srs_config::srs_resource_set, srs_config::srs_res_set_id::MAX_NOF_SRS_RES_SETS>;

    void fill_srs_res_sets(srs_set_t&             srs_res_set_list,
                           srs_config::srs_res_id res_id,
                           span<const unsigned>   slot_offsets) const;

    // Parameters that are common to all cell SRS resources.
    struct srs_cell_common {
      unsigned c_srs;
      unsigned freq_shift;
      int      p0;
    };

    // Maximum number of SRS resources that can be generated in a cell.
    // [Implementation-defined] We assume each UE has one and only one resource.
    static constexpr unsigned                    max_nof_srs_res = MAX_NOF_DU_UES;
    const du_cell_config&                        cell_cfg;
    const std::optional<tdd_ul_dl_config_common> tdd_ul_dl_cfg_common;
    // Default SRS configuration for the cell.
    const srs_config default_srs_cfg;
    srs_cell_common  srs_common_params;
    // List of all SRS resources available to the cell; these resources can be allocated over to different UEs over
    // different offsets.
    std::vector<du_srs_resource> cell_srs_res_list;
    // List of SRS resource ID and offset that can be allocated to the cell's UEs.
    std::vector<unsigned> srs_res_usage;

    std::vector<unsigned> slot_offsets;
  };

  // Contains the resources for the different cells of the DU.
  static_vector<cell_context, MAX_NOF_DU_CELLS> cells;
};

} // namespace odu
} // namespace ocudu
