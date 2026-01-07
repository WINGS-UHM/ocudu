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

#include "ocudu/cu_cp/cu_cp_types.h"
#include "ocudu/ocudulog/logger.h"

namespace ocudu {
namespace ocucp {

class du_processor_repository;
struct cu_cp_paging_message;

/// Class responsible for handling incoming paging messages and forwarding them to the appropriate DUs.
class paging_message_handler
{
public:
  paging_message_handler(du_processor_repository& dus_);

  /// Handle Paging message sent by the core network and distribute across the served DU cells.
  void handle_paging_message(const cu_cp_paging_message& msg);

private:
  bool handle_du_paging_message(du_index_t du_index, const cu_cp_paging_message& msg);

  du_processor_repository& dus;
  ocudulog::basic_logger&  logger;
};

} // namespace ocucp
} // namespace ocudu
