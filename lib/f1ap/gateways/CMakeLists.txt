#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

add_library(ocudu_f1c_gateway
        f1c_network_client_factory.cpp
        f1c_network_server_factory.cpp
        f1c_local_connector_factory.cpp)
target_include_directories(ocudu_f1c_gateway PRIVATE ..)
target_link_libraries(ocudu_f1c_gateway f1ap_asn1 ocudu_support ocudu_pcap)
