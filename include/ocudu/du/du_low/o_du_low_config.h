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

#include "ocudu/du/du_low/du_low_config.h"
#include "ocudu/fapi_adaptor/phy/phy_fapi_fastpath_adaptor_config.h"

namespace ocudu {
namespace odu {

using cell_prach_ports_entry = std::vector<uint8_t>;

/// O-RAN DU low configuration.
struct o_du_low_config {
  du_low_config du_low_cfg;
  /// FAPI adaptor configuration.
  fapi_adaptor::phy_fapi_fastpath_adaptor_config fapi_cfg;
  /// Metrics configuration. Set to \c true to enable the DU low metrics.
  bool enable_metrics;
};

/// O-RAN DU low dependencies.
struct o_du_low_dependencies {
  /// DU Low dependencies.
  du_low_dependencies du_low_deps;
  /// FAPI P5 executor.
  task_executor& fapi_p5_executor;
};

} // namespace odu
} // namespace ocudu
