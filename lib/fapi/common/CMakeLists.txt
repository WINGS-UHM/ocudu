#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

set(SOURCES
    error_indication_validator.cpp)

add_library(ocudu_fapi_common_validators STATIC ${SOURCES})
target_link_libraries(ocudu_fapi_common_validators ocudulog ocudu_fapi_message_validator_helpers)
target_include_directories(ocudu_fapi_common_validators PRIVATE ${CMAKE_SOURCE_DIR}/lib/fapi/validators_helpers)
