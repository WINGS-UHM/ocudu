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

#include "ocudu/rohc/rohc_factory.h"

namespace ocudu::rohc {

class rohc_lib_factory : public rohc_factory
{
public:
  rohc_lib_factory()  = default;
  ~rohc_lib_factory() = default;

  std::unique_ptr<rohc_compressor>   create_rohc_compressor(const rohc_config& cfg) const override;
  std::unique_ptr<rohc_decompressor> create_rohc_decompressor(const rohc_config& cfg) const override;
};

} // namespace ocudu::rohc
