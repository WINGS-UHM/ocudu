/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "ocudu/adt/span.h"
#include <array>
#include <cstdlib>

int main()
{
  std::array<int, 3> arr = {10, 20, 30};
  ocudu::span<int>   a(arr);

  ocudu::span<int> b;

  std::abort();
}
