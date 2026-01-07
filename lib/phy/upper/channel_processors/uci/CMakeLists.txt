#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

add_library(ocudu_uci_decoder STATIC
        factories.cpp
        uci_decoder_impl.cpp)
target_link_libraries(ocudu_uci_decoder ocudu_channel_coding ocuduvec)
