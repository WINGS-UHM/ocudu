// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#include "f1ap_du_ue.h"
#include "du/procedures/f1ap_du_ue_context_modification_procedure.h"
#include "du/procedures/f1ap_du_ue_context_setup_procedure.h"
#include "ocudu/f1ap/du/f1ap_du_ue_config.h"

using namespace ocudu;
using namespace ocudu::odu;

void f1ap_du_ue::handle_ue_context_modification_request(const asn1::f1ap::ue_context_mod_request_s& msg,
                                                        const f1ap_du_context&                      ctxt_)
{
  du_handler.get_ue_handler(context.ue_index)
      .schedule_async_task(launch_async<f1ap_du_ue_context_modification_procedure>(msg, *this, ctxt_));
}
