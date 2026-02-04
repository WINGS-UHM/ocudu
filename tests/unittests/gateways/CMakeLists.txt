#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

set_directory_properties(PROPERTIES LABELS "gateways")

add_executable(sctp_socket_test sctp_socket_test.cpp)
target_link_libraries(sctp_socket_test ocudu_gateway ocudulog gtest gtest_main)
gtest_discover_tests(sctp_socket_test PROPERTIES "LABELS;sctp")

add_executable(sctp_network_gateway_test
        sctp_network_server_test.cpp
        sctp_network_client_test.cpp
        sctp_network_link_test.cpp)
target_link_libraries(sctp_network_gateway_test ocudu_gateway ocudu_support ocudulog gtest gtest_main)
gtest_discover_tests(sctp_network_gateway_test PROPERTIES "LABELS;sctp")

add_executable(udp_network_gateway_test udp_network_gateway_test.cpp)
target_link_libraries(udp_network_gateway_test ocudu_gateway ocudu_support ocudulog gtest gtest_main)
gtest_discover_tests(udp_network_gateway_test PROPERTIES "LABELS;tsan")

add_executable(udp_network_gateway_pool_depletion_test udp_network_gateway_pool_depletion_test.cpp)
target_link_libraries(udp_network_gateway_pool_depletion_test ocudu_gateway ocudu_support ocudulog gtest gtest_main)
gtest_discover_tests(udp_network_gateway_pool_depletion_test PROPERTIES "LABELS;tsan")
