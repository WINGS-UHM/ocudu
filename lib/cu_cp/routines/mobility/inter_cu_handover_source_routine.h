/*
 *
 * Copyright 2021-2025 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#pragma once

#include "../../cu_up_processor/cu_up_processor_repository.h"
#include "../../du_processor/du_processor_repository.h"
#include "../../ue_manager/ue_manager_impl.h"
#include "ocudu/cu_cp/cu_cp_types.h"
#include "ocudu/ocudulog/logger.h"
#include "ocudu/support/async/async_task.h"

namespace ocudu::ocucp {

async_task<bool> start_inter_cu_handover_source_routine(ue_index_t                    ue_index,
                                                        byte_buffer                   command,
                                                        ue_manager&                   ue_mng,
                                                        du_processor_repository&      du_db,
                                                        cu_up_processor_repository&   cu_up_db,
                                                        ngap_control_message_handler& ngap,
                                                        ocudulog::basic_logger&       logger);

} // namespace ocudu::ocucp
