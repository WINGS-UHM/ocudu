/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "ocudu/adt/byte_buffer.h"
#include "ocudu/adt/byte_buffer_view.h"
#include <cstdlib>

int main()
{
  ocudu::byte_buffer a;
  (void)a.append({0x01, 0xa2, 0xf3});

  ocudu::byte_buffer b;

  ocudu::byte_buffer_view c(a);

  ocudu::byte_buffer_slice d(a);

  std::abort();
}
