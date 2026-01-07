#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#


set(SOURCES
	nrppa_ue_context_test.cpp
	)

add_executable(nrppa_test ${SOURCES})
target_include_directories(nrppa_test PRIVATE ${CMAKE_SOURCE_DIR})
set_target_properties(nrppa_test PROPERTIES UNITY_BUILD ON)
target_link_libraries(nrppa_test nrppa_test_doubles ocudu_nrppa ocudu_support nrppa_asn1 ocudu_ran ocudulog gtest gtest_main)
gtest_discover_tests(nrppa_test PROPERTIES "LABELS;nrppa")