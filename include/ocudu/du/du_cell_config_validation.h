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

#include "ocudu/du/du_cell_config.h"
#include "ocudu/support/config/validator_result.h"

namespace ocudu {
namespace odu {

/// \brief Checks whether the provided DU cell configuration is valid.
///
/// \param cell_cfg DU cell configuration.
/// \return in case an invalid parameter is detected, returns a string containing an error message.
validator_result is_du_cell_config_valid(const du_cell_config& cell_cfg);

} // namespace odu
} // namespace ocudu
