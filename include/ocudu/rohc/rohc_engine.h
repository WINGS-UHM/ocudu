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

#include "ocudu/adt/byte_buffer.h"

namespace ocudu::rohc {

class rohc_engine
{
public:
  virtual ~rohc_engine() = default;
};

class rohc_compressor
{
public:
  virtual ~rohc_compressor()                       = default;
  virtual byte_buffer compress(byte_buffer packet) = 0;
};

struct rohc_decromp_result {
  /// The decompressed packet. Empty in case of feedback-only or error.
  byte_buffer decomp_packet;
  /// Feedback packet for transmission to the peer. Empty in case of no feedback available.
  byte_buffer feedback_packet;
};

class rohc_decompressor
{
public:
  virtual ~rohc_decompressor()                               = default;
  virtual rohc_decromp_result decompress(byte_buffer packet) = 0;
};

} // namespace ocudu::rohc
