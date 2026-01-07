/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "iq_compression_death_impl.h"
#include "ocudu/support/error_handling.h"

using namespace ocudu;
using namespace ofh;

void iq_compression_death_impl::compress(span<uint8_t>                buffer,
                                         span<const cbf16_t>          iq_data,
                                         const ru_compression_params& params)
{
  report_error("Compression type '{}' is not supported", to_string(params.type));
}

void iq_compression_death_impl::decompress(span<cbf16_t>                iq_data,
                                           span<const uint8_t>          compressed_data,
                                           const ru_compression_params& params)
{
  report_error("Decompression type '{}' is not supported", to_string(params.type));
}
