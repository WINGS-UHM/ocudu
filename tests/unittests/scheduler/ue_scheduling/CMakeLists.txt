#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

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
        gtest_main)
add_test(ue_scheduler_test ue_scheduler_test)
gtest_discover_tests(ue_scheduler_test)
