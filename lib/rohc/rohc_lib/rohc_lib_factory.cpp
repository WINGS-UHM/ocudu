/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "rohc_lib_factory.h"
#include "rohc_lib_compressor.h"
#include "rohc_lib_decompressor.h"

using namespace ocudu;
using namespace ocudu::rohc;

std::unique_ptr<rohc_compressor> rohc_lib_factory::create_rohc_compressor(const rohc_config& cfg) const
{
  return std::make_unique<rohc_lib_compressor>(cfg);
}

std::unique_ptr<rohc_decompressor> rohc_lib_factory::create_rohc_decompressor(const rohc_config& cfg) const
{
  return std::make_unique<rohc_lib_decompressor>(cfg);
}
