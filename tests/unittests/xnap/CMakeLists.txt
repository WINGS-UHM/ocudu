#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

set_directory_properties(PROPERTIES LABELS "xnap")

add_executable(xn_setup_procedure_test xn_setup_procedure_test.cpp)
target_link_libraries(xn_setup_procedure_test ocudu_xnap ocudu_support ocudulog gtest gtest_main)
target_include_directories(xn_setup_procedure_test PRIVATE ${CMAKE_SOURCE_DIR})
gtest_discover_tests(xn_setup_procedure_test)

