#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

add_subdirectory(pdus)

set(SOURCES
    crc_indication_message_validator.cpp
    dl_tti_request_message_validator.cpp
    rach_indication_message_validator.cpp
    rx_data_indication_message_validator.cpp
    slot_indication_message_validator.cpp
    srs_indication_message_validator.cpp
    tx_data_request_message_validator.cpp
    uci_indication_message_validator.cpp
    ul_dci_request_message_validator.cpp
    ul_tti_request_message_validator.cpp
)

add_library(ocudu_fapi_p7_validators STATIC ${SOURCES})
target_link_libraries(ocudu_fapi_p7_validators ocudulog ocudu_fapi_message_validator_helpers ocudu_fapi_p7_pdus_validators)
target_include_directories(ocudu_fapi_p7_validators PRIVATE ${CMAKE_SOURCE_DIR}/lib/fapi/validators_helpers)
