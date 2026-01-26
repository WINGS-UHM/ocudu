#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

add_executable(f1u_cu_up_bearer_test f1u_cu_up_bearer_test.cpp)
target_link_libraries(f1u_cu_up_bearer_test ocudu_f1u_cu_up ocudu_support ocudulog gtest gtest_main)
target_include_directories(f1u_cu_up_bearer_test PRIVATE ${CMAKE_SOURCE_DIR})
add_test(f1u_cu_up_bearer_test f1u_cu_up_bearer_test)
gtest_discover_tests(f1u_cu_up_bearer_test)

add_executable(f1u_cu_up_tx_metrics_test f1u_cu_up_tx_metrics_test.cpp)
target_link_libraries(f1u_cu_up_tx_metrics_test ocudu_f1u_cu_up ocudu_support ocudulog gtest gtest_main)
target_include_directories(f1u_cu_up_tx_metrics_test PRIVATE ${CMAKE_SOURCE_DIR})
add_test(f1u_cu_up_tx_metrics_test f1u_cu_up_tx_metrics_test)
set_tests_properties(f1u_cu_up_tx_metrics_test PROPERTIES LABELS "tsan")
gtest_discover_tests(f1u_cu_up_tx_metrics_test)

add_executable(f1u_cu_up_rx_metrics_test f1u_cu_up_rx_metrics_test.cpp)
target_link_libraries(f1u_cu_up_rx_metrics_test ocudu_f1u_cu_up ocudu_support ocudulog gtest gtest_main)
target_include_directories(f1u_cu_up_rx_metrics_test PRIVATE ${CMAKE_SOURCE_DIR})
add_test(f1u_cu_up_rx_metrics_test f1u_cu_up_rx_metrics_test)
set_tests_properties(f1u_cu_up_rx_metrics_test PROPERTIES LABELS "tsan")
gtest_discover_tests(f1u_cu_up_rx_metrics_test)
