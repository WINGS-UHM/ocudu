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

#include "ocudu/cu_cp/cu_cp_configuration.h"
#include "ocudu/ran/gnb_id.h"
#include "ocudu/ran/guami.h"
#include <vector>

namespace ocudu::ocucp {

struct xnap_configuration {
  gnb_id_t                             gnb_id;
  std::vector<supported_tracking_area> tai_support_list;
  std::vector<guami_t>                 guami_list;
  transport_layer_address              peer_addr;
};

} // namespace ocudu::ocucp
