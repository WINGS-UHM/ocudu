#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

add_executable(radio_util_sample radio_util_sample.cpp)
target_link_libraries(radio_util_sample ocudu_radio ocudulog ocudu_support)

add_executable(rx_power_analyzer rx_power_analyzer.cpp)
target_link_libraries(rx_power_analyzer ocudu_radio ocudulog ocudu_support ocuduvec)
