#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

add_library(ocudu_xnc_gateway
        xnc_network_gateway_factory.cpp
        )
target_include_directories(ocudu_xnc_gateway PRIVATE ..)
target_link_libraries(ocudu_xnc_gateway f1ap_asn1 ocudu_support ocudu_pcap)
