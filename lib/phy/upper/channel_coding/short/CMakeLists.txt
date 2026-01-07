#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

add_library(ocudu_short_block STATIC short_block_encoder_impl.cpp short_block_detector_impl.cpp)
target_link_libraries(ocudu_short_block ocudu_support log_likelihood_ratio ocuduvec)