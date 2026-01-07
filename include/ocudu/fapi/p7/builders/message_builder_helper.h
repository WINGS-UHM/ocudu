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

#include "ocudu/support/ocudu_assert.h"
#include <type_traits>

namespace ocudu {
namespace fapi {

// :TODO: Review the builders documentation so it matches the UCI builder.

namespace detail {

/// \brief Sets the value of a bit in the bitmap. When enable is true, it sets the bit, otherwise it clears the bit.
/// \param[in/out] bitmap Bitmap to modify.
/// \param[in] bit Bit to change.
/// \param[in] enable Value to set. If true, sets the bit(1), otherwise clears it(0).
/// \note Use this function with integer data types, otherwise it produces undefined behaviour.
template <typename Integer>
void set_bitmap_bit(Integer& bitmap, unsigned bit, bool enable)
{
  static_assert(std::is_integral<Integer>::value, "Integral required");
  ocudu_assert(sizeof(bitmap) * 8 > bit, "Requested bit ({}), exceeds the bitmap size({})", bit, sizeof(bitmap) * 8);

  if (enable) {
    bitmap |= (1U << bit);
  } else {
    bitmap &= ~(1U << bit);
  }
}

} // namespace detail
} // namespace fapi
} // namespace ocudu
