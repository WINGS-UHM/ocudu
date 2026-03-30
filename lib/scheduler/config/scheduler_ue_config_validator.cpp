// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#include "ocudu/scheduler/config/scheduler_ue_config_validator.h"
#include "cell_configuration.h"
#include "ocudu/scheduler/config/serving_cell_config_validator.h"
#include "ocudu/support/config/validator_helpers.h"

using namespace ocudu;
using namespace config_validators;

validator_result validate_bwp_ded_cfg(const serving_cell_config& ue_cell_cfg, const cell_configuration& cell_cfg)
{
  VERIFY(ue_cell_cfg.dl_bwps.empty(), "Only init DL BWP is supported");
  const auto& ue_bwp_ded              = ue_cell_cfg.init_dl_bwp;
  const auto& expected_bwp_ded_pdcchs = cell_cfg.bwp_res[to_bwp_id(0)].dl().ded_pdcchs;
  if (ue_bwp_ded.pdcch_cfg.has_value()) {
    // If UE has been provided a dedicated PDCCH config, verify it matches one of the base ones.
    auto pdcch_it =
        std::find(expected_bwp_ded_pdcchs.begin(), expected_bwp_ded_pdcchs.end(), ue_bwp_ded.pdcch_cfg.value());
    if (pdcch_it == expected_bwp_ded_pdcchs.end()) {
      return make_unexpected("Inconsistent PDCCH config");
    }
  }
  return {};
}

error_type<std::string>
ocudu::config_validators::validate_sched_ue_creation_request_message(const sched_ue_creation_request_message& msg,
                                                                     const cell_configuration&                cell_cfg)
{
  // Verify the list of ServingCellConfig contains spCellConfig.
  VERIFY(msg.cfg.cells.has_value() and not msg.cfg.cells->empty(), "Empty list of ServingCellConfig");

  for (const ue_cell_config& cell : *msg.cfg.cells) {
    const auto& serv_cell_cfg = cell.serv_cell_cfg;
    HANDLE_ERROR(validate_pdcch_cfg(serv_cell_cfg, cell_cfg.params.dl_cfg_common));
    HANDLE_ERROR(validate_bwp_ded_cfg(serv_cell_cfg, cell_cfg));

    HANDLE_ERROR(validate_pdsch_cfg(serv_cell_cfg));

    if (serv_cell_cfg.ul_config.has_value()) {
      if (serv_cell_cfg.ul_config->init_ul_bwp.pucch_cfg.has_value() and
          serv_cell_cfg.ul_config->init_ul_bwp.srs_cfg.has_value() and
          cell_cfg.params.ul_cfg_common.init_ul_bwp.pucch_cfg_common.has_value()) {
        const pucch_config_common& pucch_cfg_common =
            cell_cfg.params.ul_cfg_common.init_ul_bwp.pucch_cfg_common.value();
        const unsigned max_pucch_payload = cell_cfg.params.init_bwp.pucch.resources.max_payload_234();
        HANDLE_ERROR(validate_pucch_cfg(serv_cell_cfg,
                                        cell_cfg.bwp_res[to_bwp_id(0)].ul().pucch.resources,
                                        pucch_cfg_common,
                                        cell_cfg.params.dl_carrier.nof_ant,
                                        max_pucch_payload));
        HANDLE_ERROR(validate_srs_cfg(serv_cell_cfg, cell_cfg.params.ul_cfg_common.init_ul_bwp.generic_params.crbs));
      }

      HANDLE_ERROR(validate_pusch_cfg(serv_cell_cfg.ul_config.value(), serv_cell_cfg.csi_meas_cfg.has_value()));
    }

    HANDLE_ERROR(validate_csi_meas_cfg(serv_cell_cfg, cell_cfg.params.tdd_cfg, cell_cfg.params.ul_cfg_common));

    // At the moment, we only support the situation where all UEs have the same NZP-CSI-RS list.
    if (serv_cell_cfg.csi_meas_cfg.has_value() and not serv_cell_cfg.csi_meas_cfg->nzp_csi_rs_res_list.empty()) {
      VERIFY(serv_cell_cfg.csi_meas_cfg->nzp_csi_rs_res_list == cell_cfg.nzp_csi_rs_list,
             "The NZP-CSI-RS Resource lists for the UE and the cell do not match");
    }
  }

  // TODO: Validate other parameters.
  return {};
}
