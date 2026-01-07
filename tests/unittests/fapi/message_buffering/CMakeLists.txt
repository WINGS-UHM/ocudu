#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

set_directory_properties(PROPERTIES LABELS "fapi")

add_executable(message_bufferer_slot_gateway_impl_test message_bufferer_slot_gateway_impl_test.cpp)
target_link_libraries(message_bufferer_slot_gateway_impl_test ocudu_support ocudu_fapi_message_builder_test_helpers ocudulog ocudu_fapi gtest gtest_main)
gtest_discover_tests(message_bufferer_slot_gateway_impl_test)
