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

#include "rrc_ue_logger.h"
#include "rrc_ue_srb_context.h"
#include "ocudu/asn1/rrc_nr/ul_ccch_msg_ies.h"
#include "ocudu/asn1/rrc_nr/ul_dcch_msg_ies.h"
#include "ocudu/ran/plmn_identity.h"
#include "ocudu/rrc/rrc_cell_context.h"
#include "ocudu/rrc/rrc_ue.h"
#include "ocudu/rrc/rrc_ue_capabilities.h"
#include "ocudu/rrc/rrc_ue_config.h"

namespace ocudu::ocucp {

/// Holds the RRC UE context used by the UE object and all its procedures.
class rrc_ue_context_t
{
public:
  rrc_ue_context_t(const ue_index_t                       ue_index_,
                   rnti_t                                 c_rnti_,
                   const rrc_cell_context&                cell_,
                   const rrc_ue_cfg_t&                    cfg_,
                   std::optional<rrc_ue_transfer_context> rrc_context_,
                   rrc_ue_logger&                         logger_);

  const ue_index_t                   ue_index; // UE index assigned by the DU processor
  rnti_t                             c_rnti;   // current C-RNTI
  rrc_cell_context                   cell;     // current cell
  const rrc_ue_cfg_t                 cfg;
  plmn_identity                      plmn_id = plmn_identity::test_value(); // PLMN identity of the UE
  rrc_state                          state   = rrc_state::idle;
  std::optional<rrc_meas_cfg>        meas_cfg;
  std::optional<uint8_t>             serving_cell_mo;
  std::optional<cu_cp_five_g_s_tmsi> five_g_s_tmsi;
  std::variant<uint64_t, asn1::fixed_bitstring<39>>
                                     setup_ue_id; ///< this is either a random value or the 5G-S-TMSI-PART1
  establishment_cause_t              connection_cause;
  std::map<srb_id_t, ue_srb_context> srbs;
  std::optional<asn1::rrc_nr::ue_cap_rat_container_list_l> capabilities_list;
  rrc_ue_capabilities_t                                    capabilities;
  std::optional<rrc_ue_transfer_context> transfer_context; // Context of old UE when created through mobility.
  byte_buffer                            cell_group_config;
  bool                                   reestablishment_ongoing = false;
  // NAS messages received while UE is in RRC Inactive, to be sent after successful resume.
  std::vector<byte_buffer> pending_dl_nas_transport_messages;
  rrc_ue_logger&           logger;
};

} // namespace ocudu::ocucp
