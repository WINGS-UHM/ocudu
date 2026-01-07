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

#include "ocudu/rrc/rrc_config.h"
#include "ocudu/rrc/rrc_du.h"
#include "ocudu/rrc/rrc_ue.h"

namespace ocudu {

namespace ocucp {

struct ue_context;

/// Create an instance of an RRC entity
std::unique_ptr<rrc_du> create_rrc_du(const rrc_cfg_t& cfg);

} // namespace ocucp

} // namespace ocudu
