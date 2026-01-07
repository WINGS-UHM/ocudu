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

#include "ocudu/fapi_adaptor/precoding_matrix_mapper.h"
#include "ocudu/fapi_adaptor/uci_part2_correspondence_mapper.h"
#include "ocudu/ran/subcarrier_spacing.h"
#include <memory>

namespace ocudu {

class task_executor;

namespace fapi {
class p7_last_request_notifier;
class p7_requests_gateway;
} // namespace fapi

namespace fapi_adaptor {

/// MAC-FAPI P7 sector fastpath adaptor configuration.
struct mac_fapi_p7_sector_fastpath_adaptor_config {
  /// Base station sector identifier.
  unsigned sector_id;
  /// Cell number of resource blocks.
  unsigned cell_nof_prbs;
  /// Subcarrier spacing as per TS38.331 Section 6.2.2.
  subcarrier_spacing scs;
  /// Log level.
  ocudulog::basic_levels log_level;
  /// Number of slots the L2 is ahead the L1.
  unsigned l2_nof_slots_ahead;
};

/// MAC-FAPI P7 sector fastpath adaptor dependencies.
struct mac_fapi_p7_sector_fastpath_adaptor_dependencies {
  /// FAPI P7 requests gateway.
  fapi::p7_requests_gateway& p7_gateway;
  /// P7 last requests notifier.
  fapi::p7_last_request_notifier& p7_last_req_notifier;
  /// Precoding matrix mapper.
  std::unique_ptr<precoding_matrix_mapper> pm_mapper;
  /// UCI Part2 mapper.
  std::unique_ptr<uci_part2_correspondence_mapper> part2_mapper;
  /// FAPI message bufferer task executor.
  task_executor* bufferer_task_executor = nullptr;
};

} // namespace fapi_adaptor
} // namespace ocudu
