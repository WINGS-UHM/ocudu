# SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
# SPDX-License-Identifier: BSD-3-Clause-Open-MPI
# Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.


set_directory_properties(PROPERTIES LABELS "support")

add_executable(synchronization_test stop_event_test.cpp sync_event_test.cpp)
target_link_libraries(synchronization_test ocudu_support ocudulog gtest gtest_main)
add_test(synchronization_test synchronization_test)
