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

#include "../ue_context/nrppa_ue_context.h"

namespace ocudu::ocucp {

/// \brief E-CID Measurement Termination, TS 38.455 section 8.2.4.
/// The purpose of E-CID Measurement Termination procedure is to terminate periodical E-CID measurements for the UE
/// performed by the NG-RAN node.
class e_cid_measurement_termination_procedure
{
public:
  e_cid_measurement_termination_procedure(ue_index_t              ue_index_,
                                          nrppa_ue_context_list&  ue_ctxt_list_,
                                          ocudulog::basic_logger& logger_);

  void operator()(coro_context<async_task<void>>& ctx);

  static const char* name() { return "E-CID Measurement Termination Procedure"; }

private:
  void terminate_periodic_measurements();

  ue_index_t              ue_index;
  nrppa_ue_context_list&  ue_ctxt_list;
  ocudulog::basic_logger& logger;

  nrppa_ue_context* ue_ctxt = nullptr;
};

} // namespace ocudu::ocucp
