/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "ocudu/support/versioning/build_info.h"
#include "hashes.h"

using namespace ocudu;

const char* ocudu::get_build_hash()
{
  return build_hash;
}

const char* ocudu::get_build_info()
{
  return build_info;
}

const char* ocudu::get_build_mode()
{
  return build_mode;
}
