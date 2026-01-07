#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

set_directory_properties(PROPERTIES LABELS "cu_up")

include_directories(${CMAKE_SOURCE_DIR})

add_executable(cu_up_test cu_up_test.cpp)
target_link_libraries(cu_up_test ocudu_cu_up ocudu_e1ap_cu_up ocudu_support ocudulog e1ap_asn1 gtest gtest_main)
target_include_directories(cu_up_test PRIVATE ${CMAKE_SOURCE_DIR})
gtest_discover_tests(cu_up_test)

add_executable(cu_up_ue_manager_test ue_manager_test.cpp)
target_link_libraries(cu_up_ue_manager_test ocudu_cu_up ocudu_support ocudulog gtest gtest_main)
target_include_directories(cu_up_ue_manager_test PRIVATE ${CMAKE_SOURCE_DIR})
gtest_discover_tests(cu_up_ue_manager_test)

add_executable(pdu_session_manager_test pdu_session_manager_test.cpp)
target_link_libraries(pdu_session_manager_test ocudu_cu_up ocudu_support e1ap_asn1 ocudulog gtest gtest_main)
target_include_directories(pdu_session_manager_test PRIVATE ${CMAKE_SOURCE_DIR})
gtest_discover_tests(pdu_session_manager_test)

add_executable(ngu_session_manager_test ngu_session_manager_test.cpp)
target_link_libraries(ngu_session_manager_test ocudu_cu_up ocudu_support ocudulog gtest gtest_main)
target_include_directories(ngu_session_manager_test PRIVATE ${CMAKE_SOURCE_DIR})
gtest_discover_tests(ngu_session_manager_test)

add_executable(cu_up_bearer_context_modification_routine_test procedures/cu_up_bearer_context_modification_routine_test.cpp)
target_link_libraries(cu_up_bearer_context_modification_routine_test ocudu_cu_up ocudu_support ocudulog gtest gtest_main)
target_include_directories(cu_up_bearer_context_modification_routine_test PRIVATE ${CMAKE_SOURCE_DIR})
gtest_discover_tests(cu_up_bearer_context_modification_routine_test)
