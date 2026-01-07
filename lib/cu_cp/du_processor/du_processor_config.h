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

#include "../cu_cp_controller/node_connection_notifier.h"
#include "du_configuration_handler.h"
#include "ocudu/cu_cp/cu_cp_configuration.h"
#include "ocudu/f1ap/cu_cp/f1ap_configuration.h"

namespace ocudu {
namespace ocucp {

struct du_processor_config_t {
  du_index_t                                du_index = du_index_t::invalid;
  const cu_cp_configuration&                cu_cp_cfg;
  ocudulog::basic_logger&                   logger         = ocudulog::fetch_basic_logger("CU-CP");
  du_connection_notifier*                   du_setup_notif = nullptr;
  std::unique_ptr<du_configuration_handler> du_cfg_hdlr;
};

} // namespace ocucp
} // namespace ocudu
