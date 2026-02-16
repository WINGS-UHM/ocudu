#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

add_library(xnap_test_helpers xnap_test_helpers.cpp)
target_include_directories(xnap_test_helpers PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(xnap_test_helpers ocudu_support xnap_asn1 ocudulog)

set(SOURCES
        xn_setup_procedure_test.cpp
        )

add_executable(xnap_test ${SOURCES})
target_include_directories(xnap_test PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(xnap_test xnap_test_helpers ocudu_xnap ocudu_support ocudulog gtest gtest_main)
gtest_discover_tests(xnap_test PROPERTIES "LABELS;xnap")
