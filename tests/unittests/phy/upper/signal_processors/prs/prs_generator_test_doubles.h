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

#include "ocudu/phy/upper/signal_processors/prs/prs_generator.h"

namespace ocudu {

/// Spy implementation of a PRS generator.
class prs_processor_spy : public prs_generator
{
  bool generate_method_called = false;

public:
  void generate(resource_grid_writer& grid, const prs_generator_configuration& config) override
  {
    generate_method_called = true;
  }

  /// Returns true if the map method has been called, false otherwise.
  bool is_generate_called() const { return generate_method_called; }
};

} // namespace ocudu
