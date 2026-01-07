/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#pragma once

#include "ocudu/radio/radio_factory.h"
#include <memory>
#include <string>

namespace ocudu {

std::unique_ptr<radio_factory> create_plugin_radio_factory(std::string driver_name);

} // namespace ocudu
