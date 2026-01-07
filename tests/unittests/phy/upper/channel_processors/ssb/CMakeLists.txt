#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

set_directory_properties(PROPERTIES LABELS "phy")

add_executable(ssb_processor_unittest ssb_processor_unittest.cpp)
target_link_libraries(ssb_processor_unittest ocudu_channel_processors ocudulog)
add_test(ssb_processor_unittest ssb_processor_unittest)
