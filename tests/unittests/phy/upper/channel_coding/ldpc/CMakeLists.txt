#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

add_executable(ldpc_chain_test ldpc_chain_test.cpp)
target_link_libraries(ldpc_chain_test ocudu_channel_coding ocudulog gtest gtest_main)
add_test(ldpc_chain_test ldpc_chain_test)
