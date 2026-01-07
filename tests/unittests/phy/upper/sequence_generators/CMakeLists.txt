#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

set_directory_properties(PROPERTIES LABELS "phy")

add_executable(pseudo_random_test pseudo_random_generator_test.cpp)
target_link_libraries(pseudo_random_test ocudu_sequence_generators ocuduvec ocudulog gtest gtest_main)
add_test(pseudo_random_test pseudo_random_test)
