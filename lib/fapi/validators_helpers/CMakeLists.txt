#
# Copyright 2021-2025 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

set(SOURCES
    validator_report_logger.cpp
)

add_library(ocudu_fapi_message_validator_helpers STATIC ${SOURCES})
target_link_libraries(ocudu_fapi_message_validator_helpers PRIVATE ocudulog)
