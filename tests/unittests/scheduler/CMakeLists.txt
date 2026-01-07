#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

set_directory_properties(PROPERTIES LABELS "sched")

include_directories(../../..)

add_subdirectory(test_utils)
add_subdirectory(support)
add_subdirectory(cell)
add_subdirectory(common_scheduling)
add_subdirectory(ue_context)
add_subdirectory(ue_scheduling)
add_subdirectory(uci_and_pucch)
add_subdirectory(policy)
add_subdirectory(config)
add_subdirectory(slicing)
add_subdirectory(srs_scheduling)

add_executable(pdcch_resource_allocator_test pdcch/pdcch_resource_allocator_test.cpp)
target_link_libraries(pdcch_resource_allocator_test
	ocudu_sched
	ocudulog
	mac_configuration_helpers
	sched_config
	scheduler_test_utils
	gtest
	gtest_main)
add_test(pdcch_resource_allocator_test pdcch_resource_allocator_test)

add_executable(scheduler_test
        scheduler_retx_test.cpp
        scheduler_ue_removal_test.cpp
        scheduler_tdd_test.cpp
        scheduler_ue_fallback_mode_test.cpp
        scheduler_ta_cmd_test.cpp
		scheduler_multi_cell_test.cpp
		scheduler_multi_slice_test.cpp
        scheduler_qos_test.cpp
        scheduler_multi_grants_test.cpp
        scheduler_multi_ue_test.cpp
		scheduler_no_ue_test.cpp
)
target_link_libraries(scheduler_test
        ocudu_sched
        sched_config
        scheduler_test_suite
        sched_test_doubles
        gtest
        gtest_main)
add_test(scheduler_test scheduler_test)

add_executable(scheduler_test_doubles_test test_doubles/test_pucch_res_test_builder_helper.cpp)
target_link_libraries(scheduler_test_doubles_test
        ocudu_sched
        sched_config
        scheduler_test_suite
        sched_test_doubles
        gtest
        gtest_main)
gtest_discover_tests(scheduler_test_doubles_test)

add_executable(scheduler_metrics_handler_test scheduler_metrics_handler_test.cpp)
target_link_libraries(scheduler_metrics_handler_test
        ocudu_sched
        sched_config
        sched_test_doubles
        gtest
        gtest_main)
add_test(scheduler_metrics_handler_test scheduler_metrics_handler_test)
