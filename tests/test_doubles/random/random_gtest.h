// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#pragma once

#include <random>
#include <vector>

namespace ocudu::test_random {

/// Fetch the seed under use for unit tests. The user can force a seed via --gtest_random_seed= command line option.
uint32_t seed();

/// Fetch thread-local random generator.
std::mt19937& tls_gen();

/// Returns a random integer with uniform distribution within the specified bounds.
template <typename Integer>
Integer uniform_int(Integer min, Integer max)
{
  return std::uniform_int_distribution<Integer>{min, max}(tls_gen());
}
template <typename Integer>
Integer uniform_int()
{
  return uniform_int(std::numeric_limits<Integer>::min(), std::numeric_limits<Integer>::max());
}

/// Return a vector of integers with specified size filled with random values.
template <typename Integer>
std::vector<Integer> vector_of_uniform_ints(size_t sz)
{
  static constexpr std::uniform_int_distribution<Integer> dist{std::numeric_limits<Integer>::min(),
                                                               std::numeric_limits<Integer>::max()};
  auto&                                                   rng = tls_gen();

  std::vector<Integer> vec(sz);
  for (unsigned i = 0; i != sz; ++i) {
    vec[i] = dist(rng);
  }
  return vec;
}

} // namespace ocudu::test_random
