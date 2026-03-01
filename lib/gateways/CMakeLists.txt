# SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
# SPDX-License-Identifier: BSD-3-Clause-Open-MPI


find_package(SCTP REQUIRED)

add_library(ocudu_gateway
        sctp_socket.cpp
        sctp_network_gateway_common_impl.cpp
        sctp_network_client_impl.cpp
        sctp_network_server_impl.cpp
        udp_network_gateway_impl.cpp
        sctp_network_client_factory.cpp
        sctp_network_server_factory.cpp
        udp_network_gateway_factory.cpp
)
target_link_libraries(ocudu_gateway ocudu_support ocudulog ${SCTP_LIBRARIES})
