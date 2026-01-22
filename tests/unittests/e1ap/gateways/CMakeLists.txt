#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

add_executable(e1_gateway_test e1_gateway_test.cpp)
target_link_libraries(e1_gateway_test
        ocudu_e1_gateway
        ocudu_gateway
        e1ap_test_helpers
        ocudu_e1ap_common
        ocudu_support
        ocudulog
        e1ap_asn1
        gtest
        gtest_main)
add_test(e1_gateway_test e1_gateway_test)
set_tests_properties(e1_gateway_test PROPERTIES LABELS "sctp")
