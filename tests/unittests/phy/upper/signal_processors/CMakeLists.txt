#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

set_directory_properties(PROPERTIES LABELS "phy")

add_executable(pss_processor_test ssb/pss_processor_test.cpp)
target_link_libraries(pss_processor_test ocudu_ssb_generators ocuduvec ocudulog)
add_test(pss_processor_test pss_processor_test)

add_executable(sss_processor_test ssb/sss_processor_test.cpp)
target_link_libraries(sss_processor_test ocudu_ssb_generators ocuduvec ocudulog)
add_test(sss_processor_test sss_processor_test)

add_executable(prs_generator_validator_test prs/prs_generator_validator_test.cpp)
target_link_libraries(prs_generator_validator_test ocudu_prs_generator ocudu_ran ocuduvec ocudu_sequence_generators ocudu_channel_precoder ocudulog gtest gtest_main)
add_test(prs_generator_validator_test prs_generator_validator_test)
