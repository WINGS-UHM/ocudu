#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

add_executable(channel_equalizer_support_test channel_equalizer_support.cpp)
target_link_libraries(channel_equalizer_support_test ocudu_channel_equalizer gtest gtest_main)
add_test(channel_equalizer_support_test channel_equalizer_support_test)
