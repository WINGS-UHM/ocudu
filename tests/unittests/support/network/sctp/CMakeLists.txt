#
# Copyright 2021-2025 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

set_directory_properties(PROPERTIES LABELS "sctp")

add_executable(sctp_socket_test sctp_socket_test.cpp)
target_link_libraries(sctp_socket_test ocudu_support ocudulog gtest gtest_main)
gtest_discover_tests(sctp_socket_test PROPERTIES "LABELS;sctp")

