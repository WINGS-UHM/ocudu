// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#pragma once

#include "../../cell/resource_grid.h"
#include "pucch_default_resource.h"
#include "ocudu/ran/bwp/bwp_configuration.h"
#include "ocudu/ran/pucch/pucch_configuration.h"

namespace ocudu {

/// Represents the time-frequency grants of a PUCCH resource.
struct pucch_grants {
  /// Time-frequency grants of the first hop.
  grant_info first_hop;
  /// Time-frequency grant of the second hop (if intra-slot frequency hopping is enabled).
  std::optional<grant_info> second_hop;

  bool operator==(const pucch_grants& other) const
  {
    return first_hop == other.first_hop and second_hop == other.second_hop;
  }
  bool operator!=(const pucch_grants& other) const { return not(*this == other); }

  /// Checks if this pucch_grants overlaps with another pucch_grants.
  bool overlaps(const pucch_grants& other) const;
};

/// Represents the relevant information of a PUCCH resource for collision checking.
struct pucch_collision_info {
  /// PUCCH format of the resource.
  pucch_format format;
  /// Multiplexing index of the resource. Resources with different multiplexing indices are orthogonal and do not
  /// collide. It is computed from different parameters depending on the format:
  ///  - Format 0: initial cyclic shift.
  ///  - Format 1: initial cyclic shift, time domain OCC index.
  ///  - Format 2/3: not multiplexed (always 0).
  ///  - Format 4: OCC index.
  unsigned multiplexing_index;
  /// Time-frequency grants of the resource.
  pucch_grants grants;

  /// Constructs the PUCCH collision info from a common PUCCH resource.
  pucch_collision_info(const pucch_default_resource& res, unsigned r_pucch, const bwp_configuration& bwp_cfg);

  /// Constructs the PUCCH collision info from a dedicated PUCCH resource.
  pucch_collision_info(const pucch_resource& res, const bwp_configuration& bwp_cfg);

  /// Checks whether this resource collides with another one.
  bool collides(const pucch_collision_info& other) const;
};

} // namespace ocudu
