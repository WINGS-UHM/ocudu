/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "ocudu/adt/strong_type.h"
#include <cstdlib>

struct tag;

int main()
{
  ocudu::strong_type<int, tag, ocudu::strong_equality> a{42};

  std::abort();
}
