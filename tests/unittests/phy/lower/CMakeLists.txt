#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

set_directory_properties(PROPERTIES LABELS "phy")

add_subdirectory(modulation)
add_subdirectory(processors)

add_executable(lower_phy_test lower_phy_test.cpp)
target_link_libraries(lower_phy_test ocudu_lower_phy ocudulog gtest gtest_main)
add_test(lower_phy_test lower_phy_test)
