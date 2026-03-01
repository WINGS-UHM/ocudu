# SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
# SPDX-License-Identifier: BSD-3-Clause-Open-MPI


set(SOURCES
        error_notifier_dispatcher.cpp
        error_notifier_dummy.cpp
        p7_indications_notifier_dispatcher.cpp
        p7_indications_notifier_dummy.cpp
        p7_last_request_notifier_dispatcher.cpp
        p7_last_request_notifier_dummy.cpp
        slot_indication_notifier_dummy.cpp
)

add_library(ocudu_fapi_decorator_helpers STATIC ${SOURCES})
