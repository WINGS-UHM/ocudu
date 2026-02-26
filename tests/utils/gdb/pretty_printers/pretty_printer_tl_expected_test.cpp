/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "ocudu/adt/expected.h"
#include <cstdlib>

enum class my_error { err1 = 42 };

int main()
{
  ocudu::expected<int, my_error> a{5};
  ocudu::expected<int, my_error> b{ocudu::make_unexpected(my_error::err1)};

  std::abort();
}
