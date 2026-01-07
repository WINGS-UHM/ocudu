/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "ocudu/rohc/rohc_support.h"

using namespace ocudu;
using namespace ocudu::rohc;

bool ocudu::rohc::rohc_supported()
{
#ifdef ENABLE_ROHC_LIB
  return true;
#else
  return false;
#endif
}
