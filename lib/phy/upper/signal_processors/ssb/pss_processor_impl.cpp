// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#include "pss_processor_impl.h"
#include "ocudu/ocuduvec/sc_prod.h"
#include "ocudu/phy/support/resource_grid_writer.h"

using namespace ocudu;

const pss_sequence_generator pss_processor_impl::sequence_generator;

void ocudu::pss_processor_impl::mapping(const std::array<cf_t, SEQUENCE_LEN>& sequence,
                                        resource_grid_writer&                 grid,
                                        const config_t&                       args) const
{
  // Calculate symbol and first subcarrier for PSS
  unsigned l = args.ssb_first_symbol + SSB_L;
  unsigned k = args.ssb_first_subcarrier + SSB_K_BEGIN;

  // For each port
  for (unsigned port : args.ports) {
    // Write in grid
    grid.put(port, l, k, sequence);
  }
}

void ocudu::pss_processor_impl::map(resource_grid_writer& grid, const config_t& config)
{
  // Generate sequence
  std::array<cf_t, SEQUENCE_LEN> sequence;
  sequence_generator.generate(sequence, config.phys_cell_id, config.amplitude);

  // Mapping to physical resources
  mapping(sequence, grid, config);
}
