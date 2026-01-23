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

namespace ocudu {
namespace phys_cell_id {

/// Number of NR N_id_1 Physical Cell Identifier (PCI) as described in TS38.211 Section 7.4.2.1 Physical-layer cell
/// identities.
constexpr unsigned NOF_NID_1 = 336;

/// Number of NR N_id_2 Physical Cell Identifier (PCI) as described in TS38.211 Section 7.4.2.1 Physical-layer cell
/// identities.
constexpr unsigned NOF_NID_2 = 3;

/// Number of NR N_id Physical Cell Identifier (PCI) as described in TS38.211 Section 7.4.2.1 Physical-layer cell
/// identities.
constexpr unsigned NOF_NID = NOF_NID_1 * NOF_NID_2;

/// Compute N_id_1 from the Physical Cell Identifier (PCI) as described in TS38.211 Section 7.4.2.1 Physical-layer cell
/// identities.
constexpr unsigned NID_1(unsigned NID)
{
  return NID / NOF_NID_2;
}

/// Compute N_id_2 from the Physical Cell Identifier (PCI) as described in TS38.211 Section 7.4.2.1 Physical-layer cell
/// identities.
constexpr unsigned NID_2(unsigned NID)
{
  return NID % NOF_NID_2;
}

} // namespace phys_cell_id
} // namespace ocudu
