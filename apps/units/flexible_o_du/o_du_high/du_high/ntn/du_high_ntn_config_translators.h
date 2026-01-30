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
#include "ocudu/ran/qos/five_qi.h"
#include "ocudu/ran/rb_id.h"
#include <map>

namespace ocudu {

namespace odu {
struct du_qos_config;
struct du_srb_config;
} // namespace odu

struct du_high_unit_cell_ntn_config;

/// Applies NTN-specific overrides to DU SRB configuration based on \c ntn_cfg.
void ntn_augment_du_srb_config(const du_high_unit_cell_ntn_config&     ntn_cfg,
                               std::map<srb_id_t, odu::du_srb_config>& srb_cfgs);

/// Applies NTN-specific overrides to DU QoS configuration based on \c ntn_cfg.
void ntn_augment_du_qos_config(const du_high_unit_cell_ntn_config&      ntn_cfg,
                               std::map<five_qi_t, odu::du_qos_config>& qos_cfgs);

} // namespace ocudu
