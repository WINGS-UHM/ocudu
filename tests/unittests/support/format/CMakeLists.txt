# SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
# SPDX-License-Identifier: BSD-3-Clause-Open-MPI
# Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.


set_directory_properties(PROPERTIES LABELS "format")

add_executable(format_optional_test format_optional_test.cpp)
target_link_libraries(format_optional_test ocudulog ocudu_support gtest gtest_main)
gtest_discover_tests(format_optional_test)
