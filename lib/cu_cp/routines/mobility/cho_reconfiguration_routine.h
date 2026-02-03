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

#include "../../cu_cp_impl_interface.h"
#include "../../ue_manager/ue_manager_impl.h"
#include "ocudu/cu_cp/cu_cp_types.h"
#include "ocudu/rrc/rrc_types.h"
#include "ocudu/support/async/async_task.h"
#include <chrono>
#include <vector>

namespace ocudu {
namespace ocucp {

/// \brief Request for CHO reconfiguration routine.
struct cu_cp_cho_reconfiguration_request {
  ue_index_t                source_ue_index = ue_index_t::invalid;
  std::chrono::milliseconds timeout         = std::chrono::milliseconds{10000};
};

/// \brief Handles the reconfiguration phase of Conditional Handover (CHO).
///
/// This routine sends the RRCReconfiguration with conditionalReconfiguration-r16
/// to the source UE and waits for the initial acknowledgment (RRCReconfigurationComplete
/// on source). After acknowledgment, the UE will evaluate conditions and eventually
/// attach to one of the target cells.
///
/// Steps:
/// 1. Build RRCReconfiguration with conditionalReconfiguration-r16 for all candidates.
/// 2. Notify CU-CP to start cho_target_routine on each target UE.
/// 3. Send via F1AP UE Context Modification without stopping source-UE data transmission.
/// 4. Wait for RRCReconfigurationComplete on SOURCE UE (acknowledgment).
/// 5. Set CHO state to executing.
class cho_reconfiguration_routine
{
public:
  cho_reconfiguration_routine(const cu_cp_cho_reconfiguration_request& request_,
                              cu_cp_ue&                                source_ue_,
                              f1ap_ue_context_manager&                 source_du_f1ap_ue_ctxt_mng_,
                              cu_cp_ue_context_manipulation_handler&   cu_cp_handler_,
                              cu_cp_ue_context_release_handler&        ue_context_release_handler_,
                              ue_manager&                              ue_mng_,
                              ocudulog::basic_logger&                  logger_);

  void operator()(coro_context<async_task<bool>>& ctx);

  static const char* name() { return "CHO Reconfiguration Routine"; }

private:
  /// \brief Build the RRC message with conditional reconfiguration for all candidates.
  /// \return Packed RRC message on success, empty buffer on failure.
  byte_buffer build_conditional_reconfiguration_message();

  /// \brief Generate the F1AP UE Context Modification request.
  void generate_ue_context_modification_request();

  /// \brief Release prepared CHO candidates and clear CHO context after failures.
  async_task<void> cleanup_targets();

  const cu_cp_cho_reconfiguration_request request;

  cu_cp_ue&                              source_ue;
  f1ap_ue_context_manager&               source_du_f1ap_ue_ctxt_mng;
  cu_cp_ue_context_manipulation_handler& cu_cp_handler;
  cu_cp_ue_context_release_handler&      ue_context_release_handler;
  ocudulog::basic_logger&                logger;

  // RRC reconfiguration context
  uint8_t     transaction_id = 0;
  byte_buffer rrc_container;

  // (sub-)routine requests
  f1ap_ue_context_modification_request ue_context_mod_request;

  // (sub-)routine results
  f1ap_ue_context_modification_response ue_context_mod_response;
  bool                                  reconfig_result = false;

  // Cleanup state for failed reconfiguration.
  cu_cp_ue_context_release_command  candidate_release_command;
  cu_cp_ue_context_release_complete candidate_release_complete;
  std::vector<ue_index_t>           candidates_to_release;
  size_t                            release_idx = 0;
};

} // namespace ocucp
} // namespace ocudu
