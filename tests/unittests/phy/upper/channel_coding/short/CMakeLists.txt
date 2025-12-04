#
# Copyright 2021-2025 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/test_data)
set(TEST_DATA_DIR ${CMAKE_CURRENT_BINARY_DIR}/test_data)

set_directory_properties(PROPERTIES LABELS "phy")

add_executable(short_block_chain_test short_block_chain_test.cpp)
target_link_libraries(short_block_chain_test ocudu_channel_coding ocudu_upper_phy_support ocudulog gtest gtest_main)
add_test(short_block_chain_test short_block_chain_test)

if (USE_PHY_TESTVECTORS)
    add_executable(short_block_encoder_test short_block_encoder_test.cpp)
    target_link_libraries(short_block_encoder_test ocudu_channel_coding ocudulog gtest gtest_main)
    add_test_vector(short_block_encoder_test short_block_encoder_test_data.tar.gz "")

    add_executable(short_block_detector_test short_block_detector_test.cpp)
    target_link_libraries(short_block_detector_test ocudu_channel_coding ocudu_upper_phy_support ocudulog gtest gtest_main)
    add_test_vector(short_block_detector_test short_block_detector_test_data.tar.gz "")
endif (USE_PHY_TESTVECTORS)
