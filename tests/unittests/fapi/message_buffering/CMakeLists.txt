# SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
# SPDX-License-Identifier: BSD-3-Clause-Open-MPI
# Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.


set_directory_properties(PROPERTIES LABELS "fapi")

add_executable(message_bufferer_slot_gateway_impl_test message_bufferer_slot_gateway_impl_test.cpp)
target_link_libraries(message_bufferer_slot_gateway_impl_test ocudu_support ocudu_fapi_message_builder_test_helpers ocudulog ocudu_fapi gtest gtest_main)
gtest_discover_tests(message_bufferer_slot_gateway_impl_test)
