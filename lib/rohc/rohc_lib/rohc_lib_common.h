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

#include "ocudu/rohc/rohc_config.h"
#include <rohc/rohc.h>
#include <rohc/rohc_comp.h>
#include <rohc/rohc_decomp.h>

namespace ocudu::rohc {

static constexpr rohc_cid_type_t rohc_lib_get_cid_type(uint16_t max_cid)
{
  return max_cid > rohc_cid_type_threshold ? rohc_cid_type_t::ROHC_LARGE_CID : rohc_cid_type_t::ROHC_SMALL_CID;
}

static constexpr rohc_profile_t to_rohc_profile_t(rohc_profile profile)
{
  switch (profile) {
    case rohc_profile::profile0x0001:
      return rohc_profile_t::ROHC_PROFILE_RTP;
    case rohc_profile::profile0x0002:
      return rohc_profile_t::ROHC_PROFILE_UDP;
    case rohc_profile::profile0x0003:
      return rohc_profile_t::ROHC_PROFILE_ESP;
    case rohc_profile::profile0x0004:
      return rohc_profile_t::ROHC_PROFILE_IP;
    case rohc_profile::profile0x0006:
      return rohc_profile_t::ROHC_PROFILE_TCP;
    case rohc_profile::profile0x0101:
      return rohc_profile_t::ROHCv2_PROFILE_IP_UDP_RTP;
    case rohc_profile::profile0x0102:
      return rohc_profile_t::ROHCv2_PROFILE_IP_UDP;
    case rohc_profile::profile0x0103:
      return rohc_profile_t::ROHCv2_PROFILE_IP_ESP;
    case rohc_profile::profile0x0104:
      return rohc_profile_t::ROHCv2_PROFILE_IP;
  }
  // We should never get here.
  return rohc_profile_t::ROHC_PROFILE_MAX;
}

} // namespace ocudu::rohc
