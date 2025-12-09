#
# Copyright 2021-2025 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

set_directory_properties(PROPERTIES LABELS "fapi")

add_executable(error_indication_validator_test error_indication_test.cpp)
target_link_libraries(error_indication_validator_test ocudu_support ocudu_fapi_message_builder_test_helpers ocudulog ocudu_fapi gtest gtest_main)
gtest_discover_tests(error_indication_validator_test ISCOVERY_TIMEOUT 15)
