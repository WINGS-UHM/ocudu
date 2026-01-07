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

#include "nlohmann/json_fwd.hpp"
#include "ocudu/adt/expected.h"
#include <string>
#include <string_view>

namespace ocudu {
namespace app_services {

/// \brief Remote command interface.
///
/// This interface allows to define commands executed by the remote control server.
class remote_command
{
public:
  /// Default destructor.
  virtual ~remote_command() = default;

  /// Returns the name of this command.
  virtual std::string_view get_name() const = 0;

  /// Returns the description of this command.
  virtual std::string_view get_description() const = 0;

  /// Execute this command using the parameters encoded in the given JSON.
  virtual error_type<std::string> execute(const nlohmann::json& json) = 0;
};

} // namespace app_services
} // namespace ocudu
