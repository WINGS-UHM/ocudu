# SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
# SPDX-License-Identifier: BSD-3-Clause-Open-MPI


set(SOURCES
        message_bufferer_decorator_impl.cpp
        message_bufferer_slot_gateway_impl.cpp
        message_bufferer_slot_gateway_task_dispatcher.cpp
        message_bufferer_slot_time_notifier_decorator.cpp)

add_library(ocudu_fapi_message_buffering STATIC ${SOURCES})
target_include_directories(ocudu_fapi_message_buffering PRIVATE ${CMAKE_SOURCE_DIR}/lib/fapi)
target_link_libraries(ocudu_fapi_message_buffering PRIVATE ocudu_fapi_decorator_helpers)
