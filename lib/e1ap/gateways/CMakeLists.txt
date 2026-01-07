#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

add_library(ocudu_e1_gateway
        e1_local_connector_factory.cpp
        e1_network_client_factory.cpp
        e1_network_server_factory.cpp)
target_link_libraries(ocudu_e1_gateway ocudu_support ocudu_e1ap_common e1ap_asn1 ocudu_pcap)
