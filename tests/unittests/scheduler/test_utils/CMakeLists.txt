#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

add_library(scheduler_test_utils
        config_generators.cpp
        indication_generators.cpp)
target_link_libraries(scheduler_test_utils ocudulog mac_configuration_helpers sched_config sched_test_doubles)

add_library(scheduler_test_suite
        scheduler_test_suite.cpp
        scheduler_output_test_helpers.cpp
        scheduler_test_simulator.cpp)
target_link_libraries(scheduler_test_suite ocudu_sched scheduler_test_utils gtest)
