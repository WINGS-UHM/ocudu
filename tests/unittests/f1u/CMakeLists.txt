#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

add_subdirectory(cu_up)
add_subdirectory(du)

add_executable(f1u_local_connector_test common/f1u_connector_test.cpp)
target_link_libraries(f1u_local_connector_test
ocudu_f1u_connector
ocudu_f1u_du
ocudu_f1u_cu_up
ocudu_support
ocudulog
gtest
gtest_main)

target_include_directories(f1u_local_connector_test PRIVATE ${CMAKE_SOURCE_DIR})
gtest_discover_tests(f1u_local_connector_test)

add_executable(f1u_cu_up_split_connector_test common/f1u_cu_split_connector_test.cpp)
target_link_libraries(f1u_cu_up_split_connector_test
ocudu_f1u_cu_up_split_connector
ocudu_f1u_cu_up
ocudu_gtpu
ocudu_support
ocudulog
gtest
gtest_main)

target_include_directories(f1u_cu_up_split_connector_test PRIVATE ${CMAKE_SOURCE_DIR})
gtest_discover_tests(f1u_cu_up_split_connector_test)

add_executable(f1u_du_split_connector_test common/f1u_du_split_connector_test.cpp)
target_link_libraries(f1u_du_split_connector_test
ocudu_f1u_du_split_connector
ocudu_f1u_du
ocudu_gtpu
ocudu_support
ocudulog
gtest
gtest_main)

target_include_directories(f1u_du_split_connector_test PRIVATE ${CMAKE_SOURCE_DIR})
gtest_discover_tests(f1u_du_split_connector_test)

add_executable(f1u_session_manager_test common/f1u_session_manager_test.cpp)
target_link_libraries(f1u_session_manager_test
ocudu_f1u_split_connector
ocudu_support
ocudulog
gtest
gtest_main)

target_include_directories(f1u_session_manager_test PRIVATE ${CMAKE_SOURCE_DIR})
gtest_discover_tests(f1u_session_manager_test)
