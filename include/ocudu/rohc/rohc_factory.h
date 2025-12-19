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

#include <memory>

namespace ocudu::rohc {

class rohc_compressor;
class rohc_decompressor;
struct rohc_config;

class rohc_factory
{
public:
  virtual ~rohc_factory()             = default;
  rohc_factory()                      = default;
  rohc_factory(const rohc_factory&)   = delete;
  void operator=(const rohc_factory&) = delete;

  virtual std::unique_ptr<rohc_compressor>   create_rohc_compressor(const rohc_config& cfg) const   = 0;
  virtual std::unique_ptr<rohc_decompressor> create_rohc_decompressor(const rohc_config& cfg) const = 0;
};

std::unique_ptr<rohc_factory> create_rohc_factory();

} // namespace ocudu::rohc
