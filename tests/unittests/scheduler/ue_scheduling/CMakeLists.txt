# SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
# SPDX-License-Identifier: BSD-3-Clause-Open-MPI
# Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.


add_executable(ue_scheduler_test
        fallback_scheduler_test.cpp
        ue_configuration_test.cpp
        ue_grid_allocator_test.cpp
        ue_link_adaptation_controller_test.cpp
        ue_harq_link_adaptation_test.cpp)
target_link_libraries(ue_scheduler_test
        ocudu_sched
        ocudulog
        mac_configuration_helpers
        scheduler_test_suite
        sched_config
        gtest
        random_gtest_main)
add_test(ue_scheduler_test ue_scheduler_test)
gtest_discover_tests(ue_scheduler_test)
