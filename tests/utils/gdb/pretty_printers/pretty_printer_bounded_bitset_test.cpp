/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "ocudu/adt/bounded_bitset.h"

int main()
{
  ocudu::bounded_bitset<5> a(4);

  a.flip(2);

  abort();
}
