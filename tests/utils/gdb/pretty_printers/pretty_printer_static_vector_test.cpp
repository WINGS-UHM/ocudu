/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "ocudu/adt/static_vector.h"

int main()
{
  ocudu::static_vector<int, 5> a;

  for (unsigned i = 0; i != 3; ++i) {
    a.push_back(i);
  }

  abort();
}
