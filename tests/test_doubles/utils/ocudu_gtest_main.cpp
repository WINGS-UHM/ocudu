// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#include "test_rng.h"
#include "ocudu/ocudulog/ocudulog.h"
#include "ocudu/support/error_handling.h"
#include <gtest/gtest.h>

/// Global base random seed that can be either set via command line (--gtest_random_seed=) or automatically generated.
static uint32_t base_seed = 0;

/// Sentinel value for \c test_counter in uninitialized state.
static constexpr uint32_t invalid_test_counter = std::numeric_limits<uint32_t>::max();

/// Global counter of tests run.
static std::atomic<uint32_t> test_counter = invalid_test_counter;

/// Current random seed, computed based on base_seed and iteration counter.
static std::atomic<uint32_t> iter_seed = 0;

namespace {

/// Integer mixing function to turn an integer into a well-scrambled bit output.
uint32_t split_mix_32(uint32_t x)
{
  x += 0x9e3779b9u;
  x = (x ^ (x >> 16)) * 0x85ebca6bu;
  x = (x ^ (x >> 13)) * 0xc2b2ae35u;
  return x ^ (x >> 16);
}

/// Called each time the seed is updated.
void update_current_seed(uint32_t iteration_counter)
{
  // Note: Apply golden ratio hashing number to spread the iteration index before combining it with base_seed.
  auto new_seed = split_mix_32(base_seed ^ (iteration_counter * 0x9e3779b9u));
  iter_seed.store(new_seed, std::memory_order_release);

  fmt::print("[   TEST   ] OCUDU Random Seed: base_seed={}, iteration={} -> iter_seed={}.\n",
             base_seed,
             iteration_counter,
             new_seed);
}

/// Setup a default log level for loggers based on a provided cmd line parameter "--log_level=", if present.
void setup_default_log_level(int argc, char** argv)
{
  // Parse logging level passed via command line arguments.
  ocudulog::basic_levels chosen_level = ocudulog::basic_levels::debug;

  for (int i = 1; i < argc; ++i) {
    std::string cmd_arg = argv[i];
    if (cmd_arg.find("--log_level") == 0) {
      // Log level specified in command line.
      size_t pos = cmd_arg.find("=");
      if (pos != std::string::npos and cmd_arg.size() > pos + 1) {
        cmd_arg               = cmd_arg.substr(pos + 1, cmd_arg.size());
        auto parsed_log_level = ocudulog::str_to_basic_level(cmd_arg);
        if (parsed_log_level.has_value()) {
          chosen_level = *parsed_log_level;
          i            = argc;
          break;
        }
      } else {
        for (; i < argc; ++i) {
          cmd_arg = argv[i];
          if (not cmd_arg.empty()) {
            auto parsed_log_level = ocudulog::str_to_basic_level(cmd_arg);
            if (parsed_log_level.has_value()) {
              chosen_level = *parsed_log_level;
              i            = argc;
              break;
            }
          }
        }
      }
    }
  }

  // Setup default log level.
  // TODO: Support setting a default log level in ocudulog instead of individual loggers.
  ocudulog::fetch_basic_logger("TEST").set_level(chosen_level);
  ocudulog::fetch_basic_logger("SCHED", true).set_level(chosen_level);
  ocudulog::fetch_basic_logger("MAC", true).set_level(chosen_level);
  ocudulog::fetch_basic_logger("RLC").set_level(chosen_level);
  ocudulog::fetch_basic_logger("DU-MNG").set_level(chosen_level);
  ocudulog::fetch_basic_logger("DU-F1").set_level(chosen_level);

  // Init ocudulog.
  ocudulog::init();
}

/// Gtest environment for setting up random seeds and log level.
class OCUDUTestEnvironment : public ::testing::Environment
{
public:
  void SetUp() override
  {
    // Setup the base seed based on which other test iteration seeds will be generated.
    // Note: We do not do this at ctor, because ::testing::UnitTest::GetInstance()->random_seed() is only set
    // afterwards.
    base_seed = ::testing::UnitTest::GetInstance()->random_seed();
  }

  void TearDown() override
  {
    // Ensure logs are flushed.
    ocudulog::flush();
  }
};

class RandomGeneratorResetListener final : public ::testing::EmptyTestEventListener
{
private:
  void OnTestIterationStart(const testing::UnitTest& unit_test, int iteration) override
  {
    // On each test iteration increment, we update the seed.
    update_current_seed(iteration);

    // Mark seed as initialized.
    test_counter.store(0, std::memory_order_release);
  }

  void OnTestStart(const testing::TestInfo& test_info) override
  {
    // On each test start, we bump the test counter.
    test_counter.fetch_add(1, std::memory_order_acq_rel);
  }

  void OnTestEnd(const testing::TestInfo& test_info) override
  {
    if (test_info.result()->Failed()) {
      ocudulog::flush();
      fmt::print(
          stderr, "[  FAILED  ] OCUDU Random Seed: base_seed={}, iter_seed={}.\n", base_seed, ocudu::test_rng::seed());
    }
  }
};

} // namespace

uint32_t ocudu::test_rng::seed()
{
  return iter_seed.load(std::memory_order_acquire);
}

std::mt19937& ocudu::test_rng::tls_gen()
{
  thread_local std::mt19937 rng(seed());
  thread_local uint32_t     last_test_counter{0};

  uint32_t cur_test_counter = test_counter.load(std::memory_order_acquire);
  report_fatal_error_if_not(cur_test_counter != invalid_test_counter,
                            "OCUDUTestEnvironment called before being setup. Are you trying to generate random "
                            "parameters before main() is called?");
  if (cur_test_counter != last_test_counter) {
    // New test started. Rewind generator back to original iteration seed.
    last_test_counter = cur_test_counter;
    rng.seed(seed());
  }
  return rng;
}

int main(int argc, char** argv)
{
  // Parse --log_level= if it exists to set the default log level.
  setup_default_log_level(argc, argv);

  // Init Gtest.
  ::testing::InitGoogleTest(&argc, argv);

  // Set up global seed.
  ::testing::AddGlobalTestEnvironment(new OCUDUTestEnvironment{});

  // Setup test random listener.
  auto& listeners = ::testing::UnitTest::GetInstance()->listeners();
  listeners.Append(new RandomGeneratorResetListener{});

  return RUN_ALL_TESTS();
}
