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

#include "ocudu/ocudulog/logger.h"
#include "ocudu/rohc/rohc_config.h"
#include "ocudu/rohc/rohc_decompressor.h"
#include <rohc/rohc.h>
#include <rohc/rohc_comp.h>
#include <rohc/rohc_decomp.h>

namespace ocudu::rohc {

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

} // namespace ocudu::rohc
