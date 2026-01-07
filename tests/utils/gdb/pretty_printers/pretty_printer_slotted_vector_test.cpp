/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "ocudu/adt/slotted_vector.h"

int main()
{
  ocudu::slotted_vector<int> a;

  a.insert(1, 3);
  a.insert(3, -5);

  abort();
}
