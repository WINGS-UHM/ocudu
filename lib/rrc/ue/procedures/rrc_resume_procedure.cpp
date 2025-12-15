/*
 *
 * Copyright 2021-2025 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "rrc_resume_procedure.h"
#include "rrc_setup_procedure.h"
#include "ue/rrc_asn1_helpers.h"
#include "ocudu/asn1/rrc_nr/dl_dcch_msg.h"
#include "ocudu/asn1/rrc_nr/nr_ue_variables.h"
#include "ocudu/cu_cp/cu_cp_types.h"
#include "ocudu/security/integrity.h"

using namespace ocudu;
using namespace ocucp;

rrc_resume_procedure::rrc_resume_procedure(const asn1::rrc_nr::rrc_resume_request_s& request_,
                                           rrc_ue_context_t&                         context_,
                                           const byte_buffer&                        du_to_cu_container_,
                                           rrc_ue_setup_proc_notifier&               rrc_ue_setup_notifier_,
                                           rrc_ue_msg4_proc_notifier&                rrc_ue_resume_notifier_,
                                           rrc_ue_control_message_handler&           srb_notifier_,
                                           rrc_ue_context_update_notifier&           cu_cp_notifier_,
                                           rrc_ue_cu_cp_ue_notifier&                 cu_cp_ue_notifier_,
                                           rrc_ue_event_notifier&                    metrics_notifier_,
                                           rrc_ue_ngap_notifier&                     ngap_notifier_,
                                           rrc_ue_event_manager&                     event_mng_,
                                           rrc_ue_logger&                            logger_) :
  resume_request(request_),
  context(context_),
  du_to_cu_container(du_to_cu_container_),
  rrc_ue_setup_notifier(rrc_ue_setup_notifier_),
  rrc_ue_resume_notifier(rrc_ue_resume_notifier_),
  srb_notifier(srb_notifier_),
  cu_cp_notifier(cu_cp_notifier_),
  cu_cp_ue_notifier(cu_cp_ue_notifier_),
  metrics_notifier(metrics_notifier_),
  ngap_notifier(ngap_notifier_),
  event_mng(event_mng_),
  logger(logger_)
{
  procedure_timeout = context.cell.timers.t311 + context.cfg.rrc_procedure_guard_time_ms;
}

void rrc_resume_procedure::operator()(coro_context<async_task<void>>& ctx)
{
  CORO_BEGIN(ctx);

  logger.log_info("\"{}\" started...", name());

  // Verify if we are in conditions for a Resume, or should opt for RRC Setup as fallback.
  if (not is_resume_accepted()) {
    CORO_AWAIT(handle_rrc_resume_fallback());
    logger.log_debug("\"{}\" finished successfully", name());
    CORO_EARLY_RETURN();
  }

  // Update the security keys and reestablish the SRBs. This must be done before the CU-CP is notified, to send the
  // correct security information to the CU-UP.
  update_security_keys();
  reestablish_srbs();

  // Notify the CU-CP about the resume request.
  request.ue_index = context.ue_index;
  request.cgi      = context.cell.cgi;
  CORO_AWAIT_VALUE(rrc_resume_context, cu_cp_notifier.on_rrc_resume_request(request));

  if (!rrc_resume_context.success) {
    CORO_AWAIT(handle_rrc_resume_fallback());
    logger.log_debug("\"{}\" finished successfully", name());
    CORO_EARLY_RETURN();
  }

  // Accept RRC Resume Request by sending RRC Resume.
  // Note: From this point we should guarantee that a Resume will be performed.

  // Create new transaction for RRC Resume.
  transaction = event_mng.transactions.create_transaction(procedure_timeout);

  // Send RRC Resume to UE.
  send_rrc_resume();

  // Await UE response.
  CORO_AWAIT(transaction);

  if (transaction.has_response()) {
    context.state = rrc_state::connected;

    logger.log_info("\"{}\" finished successfully", name());

  } else {
    logger.log_warning("\"{}\" timed out after {}ms", name(), procedure_timeout.count());
    logger.log_info("\"{}\" failed", name());
  }

  CORO_RETURN();
}

async_task<void> rrc_resume_procedure::handle_rrc_resume_fallback()
{
  context.connection_cause = establishment_cause_t::mt_access;

  return launch_async([this](coro_context<async_task<void>>& ctx) mutable {
    CORO_BEGIN(ctx);

    // Reject RRC Resume Request by sending RRC Setup.
    CORO_AWAIT(launch_async<rrc_setup_procedure>(context,
                                                 du_to_cu_container,
                                                 rrc_ue_setup_notifier,
                                                 srb_notifier,
                                                 cu_cp_notifier,
                                                 metrics_notifier,
                                                 ngap_notifier,
                                                 event_mng,
                                                 logger,
                                                 true));

    CORO_RETURN();
  });
}

bool rrc_resume_procedure::is_resume_accepted()
{
  if (context.cfg.force_resume_fallback) {
    log_rejected_resume("RRC Resumes were disabled by the app configuration");
    return false;
  }

  // Verify security context.
  return verify_security_context();
}

bool rrc_resume_procedure::verify_security_context()
{
  bool valid = false;

  // Get RX resume MAC.
  security::sec_short_mac_i resume_mac     = {};
  uint16_t                  resume_mac_int = htons(resume_request.rrc_resume_request.resume_mac_i.to_number());
  std::memcpy(resume_mac.data(), &resume_mac_int, 2);

  // Get packed varResumeMAC-Input.
  asn1::rrc_nr::var_short_mac_input_s var_resume_mac_input = {};
  var_resume_mac_input.source_pci                          = context.cell.pci;
  var_resume_mac_input.target_cell_id.from_number(context.cell.cgi.nci.value());
  var_resume_mac_input.source_c_rnti        = to_value(context.c_rnti);
  byte_buffer   var_resume_mac_input_packed = {};
  asn1::bit_ref bref(var_resume_mac_input_packed);
  var_resume_mac_input.pack(bref);

  logger.log_debug(var_resume_mac_input_packed.begin(),
                   var_resume_mac_input_packed.end(),
                   "Packed varResumeMAC-Input. Source pci={}, target cell-id=0x{:x}, source c-rnti={}",
                   var_resume_mac_input.source_pci,
                   var_resume_mac_input.target_cell_id.to_number(),
                   to_rnti(var_resume_mac_input.source_c_rnti));

  // Verify ResumeMAC-I.
  security::security_context sec_context = cu_cp_ue_notifier.get_security_context();
  if (sec_context.sel_algos.algos_selected) {
    security::sec_as_config source_as_config = sec_context.get_as_config(security::sec_domain::rrc);
    valid = security::verify_short_mac(resume_mac, var_resume_mac_input_packed, source_as_config);
    logger.log_debug("Received RRC resume request. resume_mac_valid={}", valid);
  } else {
    log_rejected_resume("UE does not have valid security context");
  }

  return valid;
}

void rrc_resume_procedure::update_security_keys()
{
  // Update security keys.
  // freq_and_timing must be present, otherwise the RRC UE would've never been created.
  uint32_t ssb_arfcn = context.cfg.meas_timings.begin()->freq_and_timing.value().carrier_freq;
  cu_cp_ue_notifier.perform_horizontal_key_derivation(context.cell.pci, ssb_arfcn);
  logger.log_debug("Refreshed keys horizontally. pci={} ssb-arfcn_f_ref={}", context.cell.pci, ssb_arfcn);
}

void rrc_resume_procedure::reestablish_srbs()
{
  // Reestablish SRBs.
  security::sec_128_as_config sec_cfg = cu_cp_ue_notifier.get_rrc_128_as_config();
  for (auto& [srb_id, srb_ctxt] : context.srbs) {
    srb_ctxt.reestablish(sec_cfg);
  }
}

void rrc_resume_procedure::send_rrc_resume()
{
  asn1::rrc_nr::dl_dcch_msg_s dl_dcch_msg;
  dl_dcch_msg.msg.set_c1().set_rrc_resume();

  fill_asn1_rrc_resume_msg(dl_dcch_msg.msg.c1().rrc_resume(), transaction.id(), rrc_resume_context);

  rrc_ue_resume_notifier.on_new_dl_dcch(srb_id_t::srb1, dl_dcch_msg);
}

void rrc_resume_procedure::log_rejected_resume(const char* cause_str)
{
  logger.log_info("Rejecting RRC Resume UE. Cause: {}. Fallback to RRC Setup Procedure...", cause_str);
}
