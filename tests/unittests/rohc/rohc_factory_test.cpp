/*
 *
 * Copyright 2021-2025 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "ocudu/ocudulog/ocudulog.h"
#include "ocudu/rohc/rohc_factory.h"
#include "ocudu/rohc/rohc_support.h"
#include <gtest/gtest.h>

using namespace ocudu;
using namespace ocudu::rohc;

/// Fixture class for ROHC factory tests
class rohc_factory_test : public ::testing::Test
{
protected:
  void SetUp() override
  {
    // init test's logger
    ocudulog::init();
    logger.set_level(ocudulog::basic_levels::debug);

    // init ROHC logger
    ocudulog::fetch_basic_logger("ROHC", false).set_level(ocudulog::basic_levels::debug);
    ocudulog::fetch_basic_logger("ROHC", false).set_hex_dump_max_size(-1);

    logger.info("ROHC factory test initialized.");
  }

  void TearDown() override
  {
    // flush logger after each test
    ocudulog::flush();
  }

  ocudulog::basic_logger& logger = ocudulog::fetch_basic_logger("TEST", false);
};

TEST_F(rohc_factory_test, create_rohc_engine_when_supported)
{
  std::unique_ptr<rohc_engine> engine = create_rohc_engine();
  if (rohc_supported()) {
    EXPECT_NE(engine, nullptr);
  } else {
    EXPECT_EQ(engine, nullptr);
  }
}

int main(int argc, char** argv)
{
  ocudulog::init();
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
