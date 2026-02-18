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

#include "ocudu/ran/plmn_identity.h"

namespace ocudu {

/// 3GPP TS 38.413 section 9.3.3.3, GUAMI (Globally unique AMF identifier)
struct guami_t {
  plmn_identity plmn = plmn_identity::test_value();
  uint16_t      amf_set_id;
  uint8_t       amf_pointer;
  uint8_t       amf_region_id;
};

} // namespace ocudu
