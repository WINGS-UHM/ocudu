/*
 *
 * Copyright 2021-2025 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#pragma once

#include "ocudu/ocudulog/logger.h"
#include "ocudu/rohc/rohc_config.h"
#include "ocudu/rohc/rohc_engine.h"
#include <rohc/rohc.h>
#include <rohc/rohc_comp.h>
#include <rohc/rohc_decomp.h>

namespace ocudu::rohc {

class rohc_lib_engine : public rohc_engine
{
public:
  rohc_lib_engine();
  ~rohc_lib_engine() = default;

private:
  ocudulog::basic_logger& logger;
};

class rohc_lib_compressor : public rohc_compressor
{
public:
  rohc_lib_compressor(rohc_config cfg_);
  ~rohc_lib_compressor();
  virtual byte_buffer compress(byte_buffer packet) override;

private:
  ocudulog::basic_logger& logger;

  rohc_config cfg;
  rohc_comp*  compressor = nullptr;

  std::vector<uint8_t> input_packet_buf;
  std::vector<uint8_t> output_packet_buf;
};

class rohc_lib_decompressor : public rohc_decompressor
{
public:
  rohc_lib_decompressor(rohc_config cfg);
  ~rohc_lib_decompressor();
  virtual rohc_decromp_result decompress(byte_buffer packet) override;

private:
  ocudulog::basic_logger& logger;

  rohc_config  cfg;
  rohc_decomp* decompressor = nullptr;

  std::vector<uint8_t> input_packet_buf;
  std::vector<uint8_t> output_packet_buf;
  std::vector<uint8_t> output_feedback_buf;
};

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
