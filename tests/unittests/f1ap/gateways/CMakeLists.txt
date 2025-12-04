#
# Copyright 2021-2025 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

add_executable(f1c_gateway_test f1c_gateway_test.cpp)
target_link_libraries(f1c_gateway_test
        ocudu_f1c_gateway
        ocudu_gateway
        f1ap_test_helpers
        ocudu_f1ap_common
        ocudu_support
        ocudulog
        f1ap_asn1
        gtest
        gtest_main)
add_test(f1c_gateway_test f1c_gateway_test)
