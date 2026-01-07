#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

set_directory_properties(PROPERTIES LABELS "phy")

add_executable(pdsch_processor_unittest pdsch_processor_unittest.cpp)
target_link_libraries(pdsch_processor_unittest ocudu_channel_processors ocudulog gtest gtest_main)
add_test(pdsch_processor_unittest pdsch_processor_unittest)

add_executable(pdsch_processor_validator_test pdsch_processor_validator_test.cpp)
target_link_libraries(pdsch_processor_validator_test
        ocudu_channel_equalizer
        ocudu_channel_precoder
        ocudu_channel_processors
        ocudu_phy_support
        ocudulog
        ocudu_upper_phy_support
        gtest
        gtest_main)
add_test(pdsch_processor_validator_test pdsch_processor_validator_test)
