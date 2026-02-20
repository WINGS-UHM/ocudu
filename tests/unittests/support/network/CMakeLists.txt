# SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
# SPDX-License-Identifier: BSD-3-Clause-Open-MPI
# Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.


set_directory_properties(PROPERTIES LABELS "io_broker;tsan")

add_executable(network_test transport_layer_address_test.cpp io_broker_epoll_test.cpp io_timer_source_test.cpp)
target_link_libraries(network_test ocudu_support ocudulog gtest gtest_main)
add_test(network_test network_test)
