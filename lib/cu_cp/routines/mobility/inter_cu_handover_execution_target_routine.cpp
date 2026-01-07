/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "inter_cu_handover_execution_target_routine.h"
#include "../pdu_session_routine_helpers.h"

using namespace ocudu;
using namespace ocucp;

inter_cu_handover_execution_target_routine::inter_cu_handover_execution_target_routine(
    cu_cp_ue*                    ue_,
    e1ap_bearer_context_manager& e1ap_,
    ngap_interface&              ngap_,
    ocudulog::basic_logger&      logger_) :
  ue(ue_), e1ap(e1ap_), ngap(ngap_), logger(logger_)
{
}

void inter_cu_handover_execution_target_routine::operator()(coro_context<async_task<void>>& ctx)
{
  CORO_BEGIN(ctx);

  if (ue == nullptr) {
    logger.warning("\"{}\" failed. Cause: UE got removed", name());
    CORO_EARLY_RETURN();
  }

  logger.debug("ue={}: \"{}\" started...", ue->get_ue_index(), name());

  // Await for NGAP DL Status transfer.
  CORO_AWAIT_VALUE(ngap_dl_ran_status, ngap.handle_dl_ran_status_transfer_required(ue->get_ue_index()));
  if (not ngap_dl_ran_status.has_value()) {
    CORO_EARLY_RETURN();
  }

  // Inform CU-UP of the current PDCP state.
  bearer_context_modification_request.ue_index = ue->get_ue_index();
  fill_e1ap_bearer_context_modification_request();
  CORO_AWAIT(e1ap.handle_bearer_context_modification_request(bearer_context_modification_request));

  // Notify RRC UE to await ReconfigurationComplete.
  if (!initialize_reconfiguration_timeout()) {
    logger.warning("ue={}: \"{}\" failed", ue->get_ue_index(), name());
    CORO_EARLY_RETURN();
  }
  CORO_AWAIT_VALUE(reconf_result,
                   ue->get_rrc_ue()->handle_handover_reconfiguration_complete_expected(0, reconf_timeout));
  if (!reconf_result) {
    logger.warning("ue={}: \"{}\" failed", ue->get_ue_index(), name());
    CORO_EARLY_RETURN();
  }

  // Send handover notify from here.
  ngap.get_ngap_control_message_handler().handle_inter_cu_ho_rrc_recfg_complete(
      ue->get_ue_index(), ue->get_rrc_ue()->get_cell_context().cgi, ue->get_rrc_ue()->get_cell_context().tac);

  CORO_RETURN();
}

void inter_cu_handover_execution_target_routine::fill_e1ap_bearer_context_modification_request()
{
  auto& ng_request = bearer_context_modification_request.ng_ran_bearer_context_mod_request;
  ng_request.emplace();
  slotted_id_vector<pdu_session_id_t, e1ap_pdu_session_res_to_modify_item>& pdu_sessions =
      ng_request->pdu_session_res_to_modify_list;
  for (const ngap_drbs_subject_to_status_transfer_item& ngap_drb :
       ngap_dl_ran_status->drbs_subject_to_status_transfer_list) {
    const up_drb_context& drb_ctx = ue->get_up_resource_manager().get_drb_context(ngap_drb.drb_id);
    pdu_session_id_t      psi     = drb_ctx.pdu_session_id;

    if (not pdu_sessions.contains(psi)) {
      e1ap_pdu_session_res_to_modify_item ps_item;
      ps_item.pdu_session_id = psi;
      pdu_sessions.emplace(psi, ps_item);
    }
    e1ap_pdu_session_res_to_modify_item& ps_item = pdu_sessions[psi];
    e1ap_drb_to_modify_item_ng_ran       drb_item;
    drb_item.drb_id = ngap_drb.drb_id;
    drb_item.pdcp_sn_status_info.emplace();

    drb_item.pdcp_sn_status_info->pdcp_status_transfer_ul.count_value.hfn     = ngap_drb.drb_status_ul.ul_count.hfn;
    drb_item.pdcp_sn_status_info->pdcp_status_transfer_ul.count_value.pdcp_sn = ngap_drb.drb_status_ul.ul_count.sn;

    drb_item.pdcp_sn_status_info->pdcp_status_transfer_dl.hfn     = ngap_drb.drb_status_dl.dl_count.hfn;
    drb_item.pdcp_sn_status_info->pdcp_status_transfer_dl.pdcp_sn = ngap_drb.drb_status_dl.dl_count.sn;

    drb_item.pdcp_cfg.emplace();
    fill_e1ap_drb_pdcp_config(*drb_item.pdcp_cfg, drb_ctx.pdcp_cfg);
    drb_item.pdcp_cfg->pdcp_reest = true;

    ps_item.drb_to_modify_list_ng_ran.emplace(drb_item.drb_id, drb_item);
  }
}

bool inter_cu_handover_execution_target_routine::initialize_reconfiguration_timeout()
{
  std::optional<std::chrono::milliseconds> t304_ms = ue->get_rrc_ue()->get_cell_context().timers.t304;
  if (!t304_ms.has_value()) {
    report_fatal_error("T304 not configured in cell context");
  }

  reconf_timeout =
      t304_ms.value() + std::chrono::milliseconds{/*We add 1s of extra time for the UE to reestablish*/ 1000};

  return true;
}
