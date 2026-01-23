/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "inter_cu_handover_target_routine.h"
#include "../../cell_meas_manager/cell_meas_manager_impl.h"
#include "../pdu_session_routine_helpers.h"
#include "ocudu/ngap/ngap_handover.h"
#include "ocudu/ran/cause/e1ap_cause_converters.h"

using namespace ocudu;
using namespace ocucp;

// This free function takes the E1AP Bearer Setup Response and pre-fills the subsequent F1AP UE Context Setup Request to
// be send to the DU.
static bool handle_bearer_context_setup_response(
    f1ap_ue_context_setup_request&                                               ue_context_setup_req,
    up_config_update&                                                            next_config,
    const slotted_id_vector<pdu_session_id_t, cu_cp_pdu_session_res_setup_item>& setup_list,
    const e1ap_bearer_context_setup_response&                                    bearer_context_setup_resp,
    up_resource_manager&                                                         up_resource_mng,
    ocudulog::basic_logger&                                                      logger);

// This free function takes the F1AP UE Context Setup Response and pre-fills the subsequent E1AP Bearer Context
// Modification to be send to the CU-UP.
static bool handle_ue_context_setup_response(e1ap_bearer_context_modification_request& bearer_ctxt_mod_request,
                                             const f1ap_ue_context_setup_response&     ue_context_setup_response,
                                             const up_config_update&                   next_config,
                                             const ocudulog::basic_logger&             logger);

inter_cu_handover_target_routine::inter_cu_handover_target_routine(
    const ngap_handover_request& request_,
    e1ap_bearer_context_manager& e1ap_bearer_ctxt_mng_,
    f1ap_ue_context_manager&     f1ap_ue_ctxt_mng_,
    cu_cp_ue_removal_handler&    ue_removal_handler_,
    ue_manager&                  ue_mng_,
    cell_meas_manager&           cell_meas_mng_,
    const security_indication_t& default_security_indication_,
    ocudulog::basic_logger&      logger_) :
  request(request_),
  e1ap_bearer_ctxt_mng(e1ap_bearer_ctxt_mng_),
  f1ap_ue_ctxt_mng(f1ap_ue_ctxt_mng_),
  ue_removal_handler(ue_removal_handler_),
  ue_mng(ue_mng_),
  cell_meas_mng(cell_meas_mng_),
  logger(logger_),
  default_security_indication(default_security_indication_)
{
  // Fill input for RRC UE creation.
  rrc_context.is_inter_cu_handover = true;
  rrc_context.sec_context          = request.security_context;
}

void inter_cu_handover_target_routine::operator()(
    coro_context<async_task<ngap_handover_resource_allocation_response>>& ctx)
{
  CORO_BEGIN(ctx);

  logger.debug("ue={}: \"{}\" started...", request.ue_index, name());

  ue = ue_mng.find_du_ue(request.ue_index);

  // Perform initial sanity checks on incoming message.
  if (!ue->get_up_resource_manager().validate_request(request.pdu_session_res_setup_list_ho_req)) {
    logger.warning("ue={}: \"{}\" failed. Cause: Invalid PduSessionResourceSetupRequest during Handover",
                   request.ue_index,
                   name());
    CORO_EARLY_RETURN(generate_handover_resource_allocation_response(false));
  }

  {
    // Calculate next user-plane configuration based on incoming setup message.
    next_config = ue->get_up_resource_manager().calculate_update(request.pdu_session_res_setup_list_ho_req);
  }

  // Prepare E1AP Bearer Context Setup Request and call E1AP notifier.
  {
    // Get security keys for Bearer Context Setup Request (RRC UE is not created yet).
    if (!ue->get_security_manager().is_security_context_initialized()) {
      logger.warning("ue={}: \"{}\" failed. Cause: Security context not initialized", request.ue_index, name());
      CORO_EARLY_RETURN(generate_handover_resource_allocation_response(false));
    }

    // Perform horizontal key derivation.
    cell_cfg = cell_meas_mng.get_cell_config(request.source_to_target_transparent_container.target_cell_id.nci);
    if (!cell_cfg.has_value() || !cell_cfg->serving_cell_cfg.pci.has_value() ||
        !cell_cfg->serving_cell_cfg.ssb_arfcn.has_value()) {
      logger.warning("ue={}: \"{}\" failed. Cause: Could not find PCI and SSB-ARFCN", request.ue_index, name());
      CORO_EARLY_RETURN(generate_handover_resource_allocation_response(false));
    }
    ue->get_security_manager().perform_horizontal_key_derivation(cell_cfg->serving_cell_cfg.pci.value(),
                                                                 cell_cfg->serving_cell_cfg.ssb_arfcn.value().value());

    if (!fill_e1ap_bearer_context_setup_request(ue->get_security_manager().get_up_as_config())) {
      logger.warning("ue={}: \"{}\" failed. Cause: Could not fill context at CU-UP", request.ue_index, name());
      CORO_EARLY_RETURN(generate_handover_resource_allocation_response(false));
    }

    // Call E1AP procedure.
    CORO_AWAIT_VALUE(bearer_context_setup_response,
                     e1ap_bearer_ctxt_mng.handle_bearer_context_setup_request(bearer_context_setup_request));

    // Handle Bearer Context Setup Response.
    if (!handle_bearer_context_setup_response(ue_context_setup_request,
                                              next_config,
                                              request.pdu_session_res_setup_list_ho_req,
                                              bearer_context_setup_response,
                                              ue->get_up_resource_manager(),
                                              logger)) {
      logger.warning("ue={}: \"{}\" failed. Cause: Could not setup bearer at CU-UP", request.ue_index, name());
      CORO_EARLY_RETURN(generate_handover_resource_allocation_response(false));
    }
  }

  // Prepare F1AP UE Context Setup Request and call F1AP notifier.
  {
    // Add remaining fields to UE Context Setup Request.
    ue_context_setup_request.ue_index      = request.ue_index;
    ue_context_setup_request.sp_cell_id    = request.source_to_target_transparent_container.target_cell_id;
    ue_context_setup_request.serv_cell_idx = 0;
    ue_context_setup_request.cu_to_du_rrc_info.ie_exts.emplace();
    ue_context_setup_request.cu_to_du_rrc_info.ie_exts.value().ho_prep_info =
        request.source_to_target_transparent_container.rrc_container.copy();
    ue_context_setup_request.cu_to_du_rrc_info.ue_cap_rat_container_list = rrc_context.ue_cap_rat_container_list.copy();

    // Generate and add Measurement Configuration to set ServingCellMO. Note that the RRC UE doesn't exist yet.
    meas_cfg = cell_meas_mng.get_measurement_config(
        request.ue_index, request.source_to_target_transparent_container.target_cell_id.nci, std::nullopt);
    if (meas_cfg.has_value()) {
      if (ue_mng.get_measurement_context(request.ue_index)
              .nci_to_meas_obj_id.find(request.source_to_target_transparent_container.target_cell_id.nci) !=
          ue_mng.get_measurement_context(request.ue_index).nci_to_meas_obj_id.end()) {
        ue_context_setup_request.serving_cell_mo = meas_obj_id_to_uint(
            ue_mng.get_measurement_context(request.ue_index)
                .nci_to_meas_obj_id.at(request.source_to_target_transparent_container.target_cell_id.nci));
      }
    }

    // Call F1AP procedure.
    CORO_AWAIT_VALUE(ue_context_setup_response,
                     f1ap_ue_ctxt_mng.handle_ue_context_setup_request(ue_context_setup_request, rrc_context));
    // Handle UE Context Setup Response.
    if (!handle_ue_context_setup_response(
            bearer_context_modification_request, ue_context_setup_response, next_config, logger)) {
      logger.warning("ue={}: \"{}\" failed. Cause: Could not setup UE context at DU", request.ue_index, name());
      CORO_AWAIT(ue_removal_handler.handle_ue_removal_request(ue_context_setup_response.ue_index));
      // Note: From this point the UE is removed and only the stored context can be accessed.
      CORO_EARLY_RETURN(generate_handover_resource_allocation_response(false));
    }
  }

  // Target UE object exists from this point on.

  // Store updated cell group config.
  {
    ue_mng.find_du_ue(request.ue_index)
        ->get_rrc_ue()
        ->update_cell_group_config(ue_context_setup_response.du_to_cu_rrc_info.cell_group_cfg.copy());
  }

  // Forward HandoverPreparationInformation to RRC.
  {
    if (!ue_mng.find_du_ue(request.ue_index)
             ->get_rrc_ue()
             ->handle_rrc_handover_preparation_info(
                 request.source_to_target_transparent_container.rrc_container.copy())) {
      logger.warning("ue={}: \"{}\" failed handle HandoverPreparationInfo at RRC", request.ue_index, name());
      CORO_AWAIT(ue_removal_handler.handle_ue_removal_request(ue_context_setup_response.ue_index));
      // Note: From this point the UE is removed and only the stored context can be accessed.
      CORO_EARLY_RETURN(generate_handover_resource_allocation_response(false));
    }
  }

  // Setup SRBs and initialize security context in RRC.
  {
    for (const auto& srb : ue_context_setup_request.srbs_to_be_setup_list) {
      create_srb(srb.srb_id);
    }
  }

  // Prepare Bearer Context Modification Request and call E1AP notifier.
  {
    // Add remaining fields to Bearer Context Modification Request.
    bearer_context_modification_request.ue_index = request.ue_index;

    // Call E1AP procedure.
    CORO_AWAIT_VALUE(
        bearer_context_modification_response,
        e1ap_bearer_ctxt_mng.handle_bearer_context_modification_request(bearer_context_modification_request));

    // Handle BearerContextModificationResponse.
    if (!bearer_context_modification_response.success) {
      logger.warning("ue={}: \"{}\" failed to modify bearer at CU-UP", request.ue_index, name());
      CORO_EARLY_RETURN(generate_handover_resource_allocation_response(false));
    }
  }

  // Prepare RRC Reconfiguration and call RRC UE notifier.
  // If default DRB is being setup, SRB2 needs to be setup as well.
  {
    if (!fill_rrc_reconfig_args(rrc_reconfig_args,
                                ue_context_setup_request.srbs_to_be_setup_list,
                                next_config.pdu_sessions_to_setup_list,
                                {} /* No DRB to be removed */,
                                ue_context_setup_response.du_to_cu_rrc_info,
                                {} /* No NAS PDUs required */,
                                ue->get_rrc_ue()->generate_meas_config(),
                                true,
                                true,
                                ue->get_security_manager().get_ncc(), /* Update keys */
                                {},
                                ue->get_security_manager().get_security_algos(),
                                logger)) {
      logger.warning("ue={}: \"{}\" failed to fill RRC Reconfiguration", request.ue_index, name());
      CORO_EARLY_RETURN(generate_handover_resource_allocation_response(false));
    }

    // Define transaction ID.
    // We set this to zero, as only in the inter CU Handover case, the first RRC transaction of the target UE is an RRC
    // Reconfiguration. When the RRC Reconfig Complete with a transaction ID = 0 is received, we will notify the NGAP to
    // trigger a HandoverNotify message.
    unsigned transaction_id = 0;

    // Get RRC Handover Command container.
    handover_command_pdu = ue->get_rrc_ue()->get_rrc_handover_command(rrc_reconfig_args, transaction_id);
  }

  CORO_RETURN(generate_handover_resource_allocation_response(true));
}

/// \brief Processes the response of a Bearer Context Setup Request.
/// \param[out] srb_setup_mod_list Reference to the successful SRB setup list.
/// \param[out] drb_setup_mod_list Reference to the successful DRB setup list.
/// \param[out] next_config Const reference to the calculated config update.
/// \param[out] ngap_setup_list Const reference to the original NGAP request.
/// \param[in] bearer_context_setup_resp Const reference to the the Bearer Context Setup Response.
/// \param[in] up_resource_mng Reference to the UP resource manager.
/// \param[in] logger Reference to the logger.
/// \return True on success, false otherwise.
static bool update_setup_list_with_bearer_ctxt_setup_response(
    std::vector<f1ap_srb_to_setup>&                                              srb_setup_mod_list,
    std::vector<f1ap_drb_to_setup>&                                              drb_setup_mod_list,
    up_config_update&                                                            next_config,
    const slotted_id_vector<pdu_session_id_t, cu_cp_pdu_session_res_setup_item>& ngap_setup_list,
    const e1ap_bearer_context_setup_response&                                    bearer_context_setup_resp,
    up_resource_manager&                                                         up_resource_mng,
    const ocudulog::basic_logger&                                                logger)
{
  // Set up SRB1 and SRB2 (this is for inter CU handover, so no SRBs are setup yet).
  for (unsigned srb_id = 1; srb_id < 3; ++srb_id) {
    f1ap_srb_to_setup srb_item;
    srb_item.srb_id = int_to_srb_id(srb_id);
    srb_setup_mod_list.push_back(srb_item);
  }

  for (const auto& e1ap_item : bearer_context_setup_resp.pdu_session_resource_setup_list) {
    const auto& psi = e1ap_item.pdu_session_id;

    // Sanity check - make sure this session ID is present in the original setup message.
    if (!ngap_setup_list.contains(e1ap_item.pdu_session_id)) {
      logger.warning("PduSessionResourceSetupRequest doesn't include setup for {}", e1ap_item.pdu_session_id);
      return false;
    }
    // Also check if PDU session is included in expected next configuration.
    if (next_config.pdu_sessions_to_setup_list.find(e1ap_item.pdu_session_id) ==
        next_config.pdu_sessions_to_setup_list.end()) {
      logger.warning("Didn't expect setup for {}", e1ap_item.pdu_session_id);
      return false;
    }

    for (const auto& e1ap_drb_item : e1ap_item.drb_setup_list_ng_ran) {
      const auto& drb_id = e1ap_drb_item.drb_id;
      if (next_config.pdu_sessions_to_setup_list.at(psi).drb_to_add.find(drb_id) ==
          next_config.pdu_sessions_to_setup_list.at(psi).drb_to_add.end()) {
        logger.warning("{} not part of next configuration", drb_id);
        return false;
      }

      // Prepare DRB item for DU.
      f1ap_drb_to_setup drb_setup_mod_item;
      if (!fill_f1ap_drb_setup_mod_item(drb_setup_mod_item,
                                        {},
                                        e1ap_item.pdu_session_id,
                                        drb_id,
                                        next_config.pdu_sessions_to_setup_list.at(psi).drb_to_add.at(drb_id),
                                        e1ap_drb_item,
                                        ngap_setup_list[e1ap_item.pdu_session_id].qos_flow_setup_request_items,
                                        logger)) {
        logger.warning("Couldn't populate DRB setup/mod item {}", e1ap_drb_item.drb_id);
        return false;
      }
      drb_setup_mod_list.push_back(drb_setup_mod_item);
    }

    // Fail on any DRB that fails to be setup.
    if (!e1ap_item.drb_failed_list_ng_ran.empty()) {
      logger.warning("Non-empty DRB failed list not supported");
      return false;
    }
  }

  return true;
}

// Same as above but taking the result from E1AP Bearer Context Setup message.
bool handle_bearer_context_setup_response(
    f1ap_ue_context_setup_request&                                               ue_context_setup_req,
    up_config_update&                                                            next_config,
    const slotted_id_vector<pdu_session_id_t, cu_cp_pdu_session_res_setup_item>& setup_list,
    const e1ap_bearer_context_setup_response&                                    bearer_context_setup_resp,
    up_resource_manager&                                                         up_resource_mng,
    ocudulog::basic_logger&                                                      logger)
{
  // Traverse setup list.
  if (!update_setup_list_with_bearer_ctxt_setup_response(ue_context_setup_req.srbs_to_be_setup_list,
                                                         ue_context_setup_req.drbs_to_be_setup_list,
                                                         next_config,
                                                         setup_list,
                                                         bearer_context_setup_resp,
                                                         up_resource_mng,
                                                         logger)) {
    return false;
  }

  return bearer_context_setup_resp.success;
}

bool handle_ue_context_setup_response(e1ap_bearer_context_modification_request& bearer_ctxt_mod_request,
                                      const f1ap_ue_context_setup_response&     ue_context_setup_resp,
                                      const up_config_update&                   next_config,
                                      const ocudulog::basic_logger&             logger)
{
  // Fail procedure if (single) DRB couldn't be setup.
  if (!ue_context_setup_resp.drbs_failed_to_be_setup_list.empty()) {
    logger.warning("Couldn't setup {} DRBs at DU", ue_context_setup_resp.drbs_failed_to_be_setup_list.size());
    return false;
  }

  if (!update_setup_list_with_ue_ctxt_setup_response(
          bearer_ctxt_mod_request, ue_context_setup_resp.drbs_setup_list, next_config, logger)) {
    return false;
  }

  // TODO: Traverse other fields.

  return ue_context_setup_resp.success;
}

bool inter_cu_handover_target_routine::fill_e1ap_bearer_context_setup_request(const security::sec_as_config& sec_info)
{
  bearer_context_setup_request.ue_index = request.ue_index;

  // Fill security info.
  bearer_context_setup_request.security_info.security_algorithm.ciphering_algo                 = sec_info.cipher_algo;
  bearer_context_setup_request.security_info.security_algorithm.integrity_protection_algorithm = sec_info.integ_algo;
  auto k_enc_buffer = byte_buffer::create(sec_info.k_enc);
  if (not k_enc_buffer.has_value()) {
    logger.warning("Unable to allocate byte_buffer");
    return false;
  }
  bearer_context_setup_request.security_info.up_security_key.encryption_key = std::move(k_enc_buffer.value());
  if (sec_info.k_int.has_value()) {
    auto k_int_buffer = byte_buffer::create(sec_info.k_int.value());
    if (not k_int_buffer.has_value()) {
      logger.warning("Unable to allocate byte_buffer");
      return false;
    }
    bearer_context_setup_request.security_info.up_security_key.integrity_protection_key =
        std::move(k_int_buffer.value());
  }

  bearer_context_setup_request.ue_dl_aggregate_maximum_bit_rate = request.ue_aggr_max_bit_rate.ue_aggr_max_bit_rate_dl;
  bearer_context_setup_request.serving_plmn = request.source_to_target_transparent_container.target_cell_id.plmn_id;
  bearer_context_setup_request.activity_notif_level = "ue"; // TODO: Remove hardcoded value
  if (bearer_context_setup_request.activity_notif_level == "ue") {
    bearer_context_setup_request.ue_inactivity_timer = ue_mng.get_ue_config().inactivity_timer;
  }

  // Fill new PDU sessions.
  fill_e1ap_pdu_session_res_to_setup_list(bearer_context_setup_request.pdu_session_res_to_setup_list,
                                          logger,
                                          next_config,
                                          request.pdu_session_res_setup_list_ho_req,
                                          ue_mng.get_ue_config(),
                                          default_security_indication);

  return true;
}

void inter_cu_handover_target_routine::create_srb(srb_id_t srb_id)
{
  // Create SRB1.
  srb_creation_message srb_msg{};
  srb_msg.ue_index        = request.ue_index;
  srb_msg.srb_id          = srb_id;
  srb_msg.pdcp_cfg        = {};
  srb_msg.enable_security = true;
  ue_mng.find_du_ue(request.ue_index)->get_rrc_ue()->create_srb(srb_msg);
}

ngap_handover_resource_allocation_response
inter_cu_handover_target_routine::generate_handover_resource_allocation_response(bool success)
{
  if (success) {
    response.success  = true;
    response.ue_index = ue->get_ue_index();

    // Prepare update for UP resource manager.
    up_config_update_result result;
    for (const auto& pdu_session_to_add : next_config.pdu_sessions_to_setup_list) {
      result.pdu_sessions_added_list.push_back(pdu_session_to_add.second);
    }
    ue->get_up_resource_manager().apply_config_update(result);

    // Fill handover request ACK.
    // Fill PDU session res admitted list.
    for (const auto& pdu_session : bearer_context_setup_response.pdu_session_resource_setup_list) {
      ngap_pdu_session_res_admitted_item admitted_item;

      // Fill PDU session ID.
      admitted_item.pdu_session_id = pdu_session.pdu_session_id;

      // Fill HO request ack transfer.
      // Fill DL NGU UP TNL info.
      admitted_item.ho_request_ack_transfer.dl_ngu_up_tnl_info = pdu_session.ng_dl_up_tnl_info;

      for (const auto& drb_setup_item : pdu_session.drb_setup_list_ng_ran) {
        // Fill QoS flow setup resp list.
        for (const auto& flow_setup_item : drb_setup_item.flow_setup_list) {
          ngap_qos_flow_item_with_data_forwarding qos_flow_item;
          // Fill QoS flow ID.
          qos_flow_item.qos_flow_id = flow_setup_item.qos_flow_id;

          admitted_item.ho_request_ack_transfer.qos_flow_setup_resp_list.push_back(qos_flow_item);
        }

        // Fill QoS flow failed to setup list.
        for (const auto& flow_failed_item : drb_setup_item.flow_failed_list) {
          cu_cp_qos_flow_with_cause_item qos_flow_item;
          // Fill QoS flow ID.
          qos_flow_item.qos_flow_id = flow_failed_item.qos_flow_id;
          // Fill Cause.
          qos_flow_item.cause = e1ap_to_ngap_cause(flow_failed_item.cause);

          admitted_item.ho_request_ack_transfer.qos_flow_failed_to_setup_list.push_back(qos_flow_item);
        }

        // Fill Data forwarding resp DRB list.
        ngap_data_forwarding_resp_drb_item drb_item;
        drb_item.drb_id = drb_setup_item.drb_id;
        admitted_item.ho_request_ack_transfer.data_forwarding_resp_drb_list.push_back(drb_item);
      }

      response.pdu_session_res_admitted_list.emplace(admitted_item.pdu_session_id, admitted_item);
    }

    // Fill PDU session res failed to setup list HO ack.
    for (const auto& pdu_session : bearer_context_setup_response.pdu_session_resource_failed_list) {
      cu_cp_pdu_session_res_setup_failed_item failed_item;

      // Fill PDU session ID.
      failed_item.pdu_session_id = pdu_session.pdu_session_id;

      // Fill NGAP HO res alloc unsuccessful transfer.
      // Fill Cause.
      failed_item.unsuccessful_transfer.cause = cause_protocol_t::unspecified;

      response.pdu_session_res_failed_to_setup_list_ho_ack.emplace(failed_item.pdu_session_id, failed_item);
    }
    // Fill Target to source transparent container.
    response.target_to_source_transparent_container.rrc_container = handover_command_pdu.copy();
  } else {
    response.success  = false;
    response.ue_index = request.ue_index;
    response.cause    = cause_protocol_t::unspecified;
  }

  return response;
}
