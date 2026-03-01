// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#include "mac_ue_removal_procedure.h"
#include "proc_logger.h"

using namespace ocudu;

void mac_ue_removal_procedure::operator()(coro_context<async_task<mac_ue_delete_response>>& ctx)
{
  CORO_BEGIN(ctx);

  logger.debug("{}: started...", mac_log_prefix(req.ue_index, req.rnti, name()));

  // > Remove UE from scheduler.
  // Note: Removing the UE from the scheduler before the MAC avoids potential race conditions (assuming the scheduler
  // doesn't allocate UEs after being removed).
  CORO_AWAIT(sched_configurator.handle_ue_removal_request(req));

  // > Remove UE and associated DL channels from the MAC DL.
  CORO_AWAIT(dl_mac.remove_ue(req));

  // > Remove UE associated UL channels from the MAC UL.
  CORO_AWAIT(ul_mac.remove_ue(req));

  // > Enqueue UE deletion
  ctrl_mac.remove_ue(req.ue_index);

  logger.info("{}: finished successfully", mac_log_prefix(req.ue_index, req.rnti, name()));

  // 4. Signal end of procedure and pass response
  CORO_RETURN(mac_ue_delete_response{true});
}
