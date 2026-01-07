#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

add_executable(sched_ue_context_test
        ue_drx_controller_test.cpp
        ue_repository_test.cpp
        logical_channel_system_test.cpp
        ta_management_system_test.cpp
        pucch_power_control_test.cpp
        pusch_power_control_test.cpp)
target_link_libraries(sched_ue_context_test
        sched_cell
        sched_support
        sched_ue_context
        sched_test_doubles
        scheduler_test_utils
        ocudu_support
        ocudulog
        gtest
        gtest_main)
add_test(sched_ue_context_test sched_ue_context_test)
