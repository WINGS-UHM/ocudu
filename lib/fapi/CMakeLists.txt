# SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
# SPDX-License-Identifier: BSD-3-Clause-Open-MPI


set(LOGGERS_SOURCES
        message_loggers.cpp
)

add_library(ocudu_fapi_message_loggers STATIC ${LOGGERS_SOURCES})
target_link_libraries(ocudu_fapi_message_loggers ocudu_ran ocudulog)
