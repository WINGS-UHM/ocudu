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

#include <cstdint>

namespace ocudu {
namespace ocucp {

// LMF UE measurement ID.
enum class lmf_ue_meas_id_t : uint16_t { min = 1, max = 256 };

/// Convert lmf_ue_meas_id type to integer.
inline uint16_t lmf_ue_meas_id_to_uint(lmf_ue_meas_id_t id)
{
  return static_cast<uint16_t>(id);
}

/// Convert integer to lmf_ue_meas_id type.
inline lmf_ue_meas_id_t uint_to_lmf_ue_meas_id(uint16_t id)
{
  return static_cast<lmf_ue_meas_id_t>(id);
}

// RAN UE measurement ID.
enum class ran_ue_meas_id_t : uint16_t { min = 1, max = 256 };

/// Convert ran_ue_meas_id type to integer.
inline uint16_t ran_ue_meas_id_to_uint(ran_ue_meas_id_t id)
{
  return static_cast<uint16_t>(id);
}

/// Convert integer to ran_ue_meas_id type.
inline ran_ue_meas_id_t uint_to_ran_ue_meas_id(uint16_t id)
{
  return static_cast<ran_ue_meas_id_t>(id);
}

} // namespace ocucp
} // namespace ocudu
