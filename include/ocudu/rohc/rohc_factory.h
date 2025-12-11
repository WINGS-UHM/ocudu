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

#include "ocudu/rohc/rohc_compressor.h"
#include "ocudu/rohc/rohc_config.h"
#include "ocudu/rohc/rohc_decompressor.h"
#include <memory>

namespace ocudu::rohc {

std::unique_ptr<rohc_compressor>   create_rohc_compressor(const rohc_config& cfg);
std::unique_ptr<rohc_decompressor> create_rohc_decompressor(const rohc_config& cfg);

} // namespace ocudu::rohc
