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

#include "ocudu/security/ciphering_engine.h"
#include "ocudu/security/security.h"
#include "ocudu/security/ssl.h"

namespace ocudu::security {

class ciphering_engine_nea2 final : public ciphering_engine
{
public:
  ciphering_engine_nea2(sec_128_key k_128_enc_, uint8_t bearer_id_, security_direction direction_);
  ~ciphering_engine_nea2() override = default;

  security_result apply_ciphering(byte_buffer buf, size_t offset, uint32_t count) override;

private:
  uint8_t            bearer_id;
  security_direction direction;
  sec_128_key        k_128_enc;

  aes_context ctx;

  ocudulog::basic_logger& logger;
};

} // namespace ocudu::security
