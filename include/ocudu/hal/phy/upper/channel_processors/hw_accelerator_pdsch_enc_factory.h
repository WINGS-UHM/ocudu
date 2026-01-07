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

#include "ocudu/hal/phy/upper/channel_processors/hw_accelerator_pdsch_enc.h"

namespace ocudu {
namespace hal {

/// PDSCH encoder hardware accelerator factory.
class hw_accelerator_pdsch_enc_factory
{
public:
  virtual ~hw_accelerator_pdsch_enc_factory()                = default;
  virtual std::unique_ptr<hw_accelerator_pdsch_enc> create() = 0;
};

} // namespace hal
} // namespace ocudu
