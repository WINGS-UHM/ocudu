// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#include "test_rng_seed.h"
#include "ocudu/ocudulog/ocudulog.h"
#include "ocudu/support/error_handling.h"
#include <gtest/gtest.h>

namespace {

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
class OCUDUTestEnvironment final : public ::testing::Environment
{
public:
  OCUDUTestEnvironment()
  {
    // Note: ::testing::UnitTest::GetInstance()->random_seed()  does not match the seed passed by --gtest_random_seed=
    ocudu::test_rng::init_base_seed(::testing::GTEST_FLAG(random_seed));
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
    last_iteration = iteration;
    ocudu::test_rng::advance_iter_seed(iteration);
  }

  void OnTestStart(const testing::TestInfo& test_info) override { ocudu::test_rng::rewind_rng(); }

  void OnTestEnd(const testing::TestInfo& test_info) override
  {
    if (test_info.result()->Failed()) {
      ocudulog::flush();
      const uint32_t iter_seed = ocudu::test_rng::seed();
      fmt::print(stderr,
                 "[  FAILED  ] OCUDU Random Seed: base_seed={}, iteration={}, iter_seed={}.\n",
                 ocudu::test_rng::base_seed(),
                 last_iteration,
                 iter_seed);
      if (last_iteration > 0) {
        fmt::print(stderr,
                   "[  FAILED  ] Note: To reproduce iter_seed={} at iteration=0, use base_seed={}.\n",
                   iter_seed,
                   ocudu::test_rng::compute_base_seed_at_iter0(last_iteration));
      }
    }
  }

  int last_iteration = 0;
};

} // namespace

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
