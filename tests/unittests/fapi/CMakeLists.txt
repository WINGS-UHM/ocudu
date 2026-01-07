#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

add_subdirectory(common)
add_subdirectory(message_buffering)
add_subdirectory(p7)

add_library(ocudu_fapi_message_builder_test_helpers STATIC message_builder_helpers.cpp)
