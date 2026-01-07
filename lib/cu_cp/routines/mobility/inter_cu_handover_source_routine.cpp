/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "inter_cu_handover_source_routine.h"

using namespace ocudu;
using namespace ocucp;

inter_cu_handover_source_routine::inter_cu_handover_source_routine(ue_index_t                    ue_index_,
                                                                   byte_buffer                   command_,
                                                                   ue_manager&                   ue_mng_,
                                                                   du_processor_repository&      du_db_,
                                                                   cu_up_processor_repository&   cu_up_db_,
                                                                   ngap_control_message_handler& ngap_,
                                                                   ocudulog::basic_logger&       logger_) :
  ue_index(ue_index_),
  command(std::move(command_)),
  ue_mng(ue_mng_),
  du_db(du_db_),
  cu_up_db(cu_up_db_),
  ngap(ngap_),
  logger(logger_)
{
}

void inter_cu_handover_source_routine::operator()(coro_context<async_task<bool>>& ctx)
{
  CORO_BEGIN(ctx);

  if (ue_mng.find_du_ue(ue_index) == nullptr) {
    CORO_EARLY_RETURN(false);
  }

  // Unpack Handover Command PDU at RRC, to get RRC Reconfig PDU.
  ho_reconfig_pdu = ue_mng.find_du_ue(ue_index)->get_rrc_ue()->handle_rrc_handover_command(std::move(command));
  if (ho_reconfig_pdu.empty()) {
    logger.warning("ue={}: Could not unpack Handover Command PDU", ue_index);
    CORO_EARLY_RETURN(false);
  }

  // Send RRC Reconfiguration to UE.
  ue_context_mod_request.ue_index                 = ue_index;
  ue_context_mod_request.drbs_to_be_released_list = ue_mng.find_du_ue(ue_index)->get_up_resource_manager().get_drbs();
  ue_context_mod_request.rrc_container            = ho_reconfig_pdu.copy();

  CORO_AWAIT_VALUE(ue_context_mod_response,
                   du_db.get_du_processor(ue_mng.find_du_ue(ue_index)->get_du_index())
                       .get_f1ap_handler()
                       .handle_ue_context_modification_request(ue_context_mod_request));

  if (not ue_context_mod_response.success) {
    CORO_EARLY_RETURN(false);
  }

  // Transfer PDCP state.
  // Get PDCP state from CU-UP.
  fill_e1ap_bearer_modification_request_pdcp_sn_query();

  CORO_AWAIT_VALUE(bearer_mod_resp,
                   cu_up_db.find_cu_up_processor(ue_mng.find_du_ue(ue_index)->get_cu_up_index())
                       ->get_e1ap_bearer_context_manager()
                       .handle_bearer_context_modification_request(bearer_mod_req));

  if (not bearer_mod_resp.success) {
    CORO_EARLY_RETURN(false);
  }

  // Send PDCP state to AMF.
  if (not fill_ngap_ul_ran_status_transfer()) {
    CORO_EARLY_RETURN(false);
  }
  ngap.handle_ul_ran_status_transfer(ul_ran_status_transfer);

  CORO_RETURN(true);
}

void inter_cu_handover_source_routine::fill_e1ap_bearer_modification_request_pdcp_sn_query()
{
  bearer_mod_req.ue_index = ue_index;
  bearer_mod_req.ng_ran_bearer_context_mod_request.emplace();
  const auto& pdu_sessions = ue_mng.find_du_ue(ue_index)->get_up_resource_manager().get_pdu_sessions();
  for (const auto& pdu_session_id : pdu_sessions) {
    e1ap_pdu_session_res_to_modify_item e1ap_pdu_session_item;
    e1ap_pdu_session_item.pdu_session_id = pdu_session_id;

    const up_pdu_session_context& pdu_session =
        ue_mng.find_du_ue(ue_index)->get_up_resource_manager().get_pdu_session_context(pdu_session_id);
    // Fill DRBs
    for (const std::pair<const drb_id_t, up_drb_context>& drb : pdu_session.drbs) {
      e1ap_drb_to_modify_item_ng_ran drb_to_mod;
      drb_to_mod.drb_id                 = drb.first;
      drb_to_mod.pdcp_sn_status_request = true;
      e1ap_pdu_session_item.drb_to_modify_list_ng_ran.emplace(drb.first, drb_to_mod);
    }
    bearer_mod_req.ng_ran_bearer_context_mod_request->pdu_session_res_to_modify_list.emplace(pdu_session_id,
                                                                                             e1ap_pdu_session_item);
  }
}

bool inter_cu_handover_source_routine::fill_ngap_ul_ran_status_transfer()
{
  ul_ran_status_transfer.ue_index = ue_index;

  for (const auto& pdu_session_modified : bearer_mod_resp.pdu_session_resource_modified_list) {
    const up_pdu_session_context& pdu_session_ctx =
        ue_mng.find_du_ue(ue_index)->get_up_resource_manager().get_pdu_session_context(
            pdu_session_modified.pdu_session_id);
    for (const e1ap_drb_modified_item_ng_ran& drb_modified : pdu_session_modified.drb_modified_list_ng_ran) {
      if (drb_modified.pdcp_sn_status_info.has_value()) {
        auto drb_ctx = pdu_session_ctx.drbs.find(drb_modified.drb_id);
        if (drb_ctx == pdu_session_ctx.drbs.end()) {
          return false;
        }
        ngap_drbs_subject_to_status_transfer_item drb_item;
        drb_item.drb_id                     = drb_modified.drb_id;
        drb_item.drb_status_dl.sn_size      = drb_ctx->second.pdcp_cfg.tx.sn_size;
        drb_item.drb_status_dl.dl_count.hfn = drb_modified.pdcp_sn_status_info->pdcp_status_transfer_dl.hfn;
        drb_item.drb_status_dl.dl_count.sn  = drb_modified.pdcp_sn_status_info->pdcp_status_transfer_dl.pdcp_sn;
        drb_item.drb_status_ul.sn_size      = drb_ctx->second.pdcp_cfg.rx.sn_size;
        drb_item.drb_status_ul.ul_count.hfn = drb_modified.pdcp_sn_status_info->pdcp_status_transfer_ul.count_value.hfn;
        drb_item.drb_status_ul.ul_count.sn =
            drb_modified.pdcp_sn_status_info->pdcp_status_transfer_ul.count_value.pdcp_sn;
        ul_ran_status_transfer.drbs_subject_to_status_transfer_list.emplace(drb_modified.drb_id, drb_item);
      }
    }
  }
  return true;
}
