#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

set_directory_properties(PROPERTIES LABELS "phy")

add_executable(evm_calculator_test evm_calculator_test.cpp)
target_link_libraries(evm_calculator_test ocudu_channel_modulation ocuduvec ocudulog gtest gtest_main)
add_test(evm_calculator_test evm_calculator_test)
