// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI

#pragma once

#include "ocudu/fapi/common/error_indication_notifier.h"

namespace ocudu {
namespace fapi {

/// Dummy FAPI error message notifier that will close the application if its methods are called.
class error_notifier_dummy : public error_indication_notifier
{
public:
  // See interface for documentation.
  void on_error_indication(const error_indication& msg) override;
};

} // namespace fapi
} // namespace ocudu
