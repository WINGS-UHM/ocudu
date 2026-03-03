// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#include "random_gtest.h"
#include "ocudu/ocudulog/ocudulog.h"
#include "ocudu/support/error_handling.h"
#include <gtest/gtest.h>

/// Global base random seed that can be either set via command line (--gtest_random_seed=) or automatically generated.
static uint32_t base_seed = 0;

/// Sentinel value for \c test_counter in uninitialized state.
constexpr uint32_t invalid_test_counter = std::numeric_limits<uint32_t>::max();

/// Global counter of tests run.
static std::atomic<uint32_t> test_counter = invalid_test_counter;

namespace {

/// Gtest environment for setting up random seeds.
class RandomSeedEnvironment : public ::testing::Environment
{
public:
  RandomSeedEnvironment()
  {
    base_seed = GTEST_FLAG_GET(random_seed);
    if (base_seed == 0) {
      // When seed == 0, it means that the user did not explicitly set a seed (according to gtest). We generate one.
      base_seed = std::random_device{}();
    }
    test_counter.store(0, std::memory_order_release);
    fmt::print("[  TEST  ] RANDOM SEED: {}\n", base_seed);
  }
};

class RandomGeneratorResetListener final : public ::testing::EmptyTestEventListener
{
private:
  void OnTestStart(const testing::TestInfo& test_info) override
  {
    // On each test start, we bump the test counter.
    test_counter.fetch_add(1, std::memory_order_acq_rel);
  }

  void OnTestEnd(const testing::TestInfo& test_info) override
  {
    ocudulog::flush();
    if (test_info.result()->Failed()) {
      fmt::print(stderr, "[  FAILED  ] Seed: {}\n", ocudu::test_random::seed());
    }
  }
};

} // namespace

uint32_t ocudu::test_random::seed()
{
  return base_seed;
}

std::mt19937& ocudu::test_random::tls_gen()
{
  thread_local std::mt19937 rng(seed());
  thread_local uint32_t     last_test_counter{0};

  uint32_t cur_test_counter = test_counter.load(std::memory_order_acquire);
  report_fatal_error_if_not(cur_test_counter != invalid_test_counter, "RandomSeedEnvironment has not been setup");
  if (cur_test_counter != last_test_counter) {
    // New test started. Reset generator.
    last_test_counter = cur_test_counter;
    rng.seed(cur_test_counter);
  }
  return rng;
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);

  // Set up global seed.
  ::testing::AddGlobalTestEnvironment(new RandomSeedEnvironment{});

  // Setup test random listener.
  auto& listeners = ::testing::UnitTest::GetInstance()->listeners();
  listeners.Append(new RandomGeneratorResetListener{});

  return RUN_ALL_TESTS();
}
