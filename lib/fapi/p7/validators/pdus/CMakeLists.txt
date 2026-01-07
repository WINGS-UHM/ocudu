#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

set(SOURCES
        dl_csi_pdu.cpp
        dl_pdcch_pdu.cpp
        dl_pdsch_pdu.cpp
        dl_prs_pdu.cpp
        dl_ssb_pdu.cpp
        tx_precoding_and_beamforming_pdu.cpp
        uci_pdus.cpp
        ul_prach_pdu.cpp
        ul_pucch_pdu.cpp
        ul_pusch_pdu.cpp
        ul_srs_pdu.cpp
)

add_library(ocudu_fapi_p7_pdus_validators STATIC ${SOURCES})
target_link_libraries(ocudu_fapi_p7_pdus_validators ocudu_fapi_message_validator_helpers)
target_include_directories(ocudu_fapi_p7_pdus_validators PRIVATE ${CMAKE_SOURCE_DIR}/lib/fapi/validators_helpers)
