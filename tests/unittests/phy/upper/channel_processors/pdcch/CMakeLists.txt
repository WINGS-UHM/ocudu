#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

set_directory_properties(PROPERTIES LABELS "phy")

add_executable(pdcch_processor_unittest pdcch_processor_unittest.cpp)
target_link_libraries(pdcch_processor_unittest ocudu_channel_processors ocudulog)
add_test(pdcch_processor_unittest pdcch_processor_unittest)
target_include_directories(pdcch_processor_unittest PRIVATE
        ${CMAKE_SOURCE_DIR}/tests/unittests/phy/support
        ${CMAKE_SOURCE_DIR}/tests/unittests/phy/upper/signal_processors/pdcch)
