#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

set(SOURCES
        logging_decorator_impl.cpp
        logging_error_notifier_decorator.cpp
        logging_p7_indications_notifier_decorator.cpp
        logging_p7_requests_gateway_decorator.cpp
        logging_p7_slot_indication_notifier_decorator.cpp
        message_loggers.cpp
)

add_library(ocudu_fapi_loggers STATIC ${SOURCES})
target_include_directories(ocudu_fapi_loggers PRIVATE ${CMAKE_SOURCE_DIR}/lib/fapi)
target_link_libraries(ocudu_fapi_loggers ocudulog ocudu_fapi_decorator_helpers)
