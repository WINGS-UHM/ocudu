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

#include "ocudu/adt/byte_buffer.h"

namespace ocudu::rohc {

class rohc_compressor
{
public:
  virtual ~rohc_compressor()                                = default;
  virtual byte_buffer compress(byte_buffer packet)          = 0;
  virtual bool        handle_feedback(byte_buffer feedback) = 0;
};

} // namespace ocudu::rohc
