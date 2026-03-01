# SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
# SPDX-License-Identifier: BSD-3-Clause-Open-MPI
# Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.


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
