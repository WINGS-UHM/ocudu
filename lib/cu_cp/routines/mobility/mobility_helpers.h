// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#pragma once

#include "../../cu_cp_impl_interface.h"
#include "../../ue_manager/ue_manager_impl.h"
#include "../../up_resource_manager/up_resource_manager_impl.h"
#include "ocudu/cu_cp/cu_cp_intra_cu_ho_types.h"
#include "ocudu/e1ap/cu_cp/e1ap_cu_cp_bearer_context_update.h"
#include "ocudu/f1ap/cu_cp/f1ap_cu_ue_context_update.h"

namespace ocudu {
namespace ocucp {

/// \brief Handle UE context setup response from target DU and prefills the Bearer context modification.
bool handle_context_setup_response(cu_cp_intra_cu_handover_response&         response_msg,
                                   e1ap_bearer_context_modification_request& bearer_context_modification_request,
                                   const f1ap_ue_context_setup_response&     target_ue_context_setup_response,
                                   up_config_update&                         next_config,
                                   const ocudulog::basic_logger&             logger,
                                   bool                                      reestablish_pdcp);

/// \brief Handler Bearer context modification response from CU-UP and prefill UE context modification for source DU.
bool handle_bearer_context_modification_response(
    cu_cp_intra_cu_handover_response&                response_msg,
    f1ap_ue_context_modification_request&            source_ue_context_mod_request,
    const e1ap_bearer_context_modification_response& bearer_context_modification_response,
    up_config_update&                                next_config,
    const ocudulog::basic_logger&                    logger);

/// \brief Cancel each non-winning CHO candidate's RRC reconfiguration transaction. Target routines observe the
/// cancellation and self-release. Candidates with invalid ue_index, candidates that alias the source UE, and the
/// winner are skipped.
/// \param[in] source_ue Source UE holding the CHO context with candidates.
/// \param[in] ue_mng UE manager for candidate lookups.
/// \param[in] winner_ue_index UE index of the CHO winner to exclude. Pass ue_index_t::invalid to cancel all
/// non-source candidates (e.g. CHO cancellation on execution timeout).
/// \return Number of transactions cancelled.
unsigned
cancel_cho_candidates(cu_cp_ue& source_ue, ue_manager& ue_mng, ue_index_t winner_ue_index = ue_index_t::invalid);

} // namespace ocucp
} // namespace ocudu
