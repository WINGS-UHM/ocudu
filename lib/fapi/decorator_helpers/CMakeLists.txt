#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

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
