/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#pragma once

#include "ocudu/fapi/common/error_indication.h"
#include "ocudu/fapi/p7/messages/crc_indication.h"
#include "ocudu/fapi/p7/messages/dl_tti_request.h"
#include "ocudu/fapi/p7/messages/rach_indication.h"
#include "ocudu/fapi/p7/messages/rx_data_indication.h"
#include "ocudu/fapi/p7/messages/slot_indication.h"
#include "ocudu/fapi/p7/messages/srs_indication.h"
#include "ocudu/fapi/p7/messages/tx_data_request.h"
#include "ocudu/fapi/p7/messages/uci_indication.h"
#include "ocudu/fapi/p7/messages/ul_dci_request.h"
#include "ocudu/fapi/p7/messages/ul_tti_request.h"

namespace unittest {
/// Builds and returns a valid CRC.indication message. Every parameter is within the range defined in SCF-222 v4.0
/// Section 3.4.8.
ocudu::fapi::crc_indication build_valid_crc_indication();

/// Builds and returns a valid DL CSI-RS pdu. Every parameter is within the range defined in SCF-222 v4.0
/// Section 3.4.2.3.
ocudu::fapi::dl_csi_rs_pdu build_valid_dl_csi_pdu();

/// Builds and returns a valid DL PDCCH PDU. Every parameter is within the range defined in SCF-222 v4.0
/// Section 3.4.2.1.
ocudu::fapi::dl_pdcch_pdu build_valid_dl_pdcch_pdu();

/// Builds and returns a valid DL PDSCH PDU. Every parameter is within the range defined in SCF-222 v4.0
/// Section 3.4.2.2.
ocudu::fapi::dl_pdsch_pdu build_valid_dl_pdsch_pdu();

/// Builds and returns a valid DL PRS pdu. Every parameter is within the range defined in SCF-222 v8.0
/// Section 3.4.2.4a.
ocudu::fapi::dl_prs_pdu build_valid_dl_prs_pdu();

/// Builds and returns a valid DL SSB pdu. Every parameter is within the range defined in SCF-222 v4.0
/// Section 3.4.2.4.
ocudu::fapi::dl_ssb_pdu build_valid_dl_ssb_pdu();

/// Builds and returns a valid DL TTI request message. Every parameter is within the range defined in SCF-222 v4.0
/// Section 3.4.2.
ocudu::fapi::dl_tti_request build_valid_dl_tti_request();

/// Builds and returns a valid ERROR.indication message. Every parameter is within the range defined in SCF-222 v4.0
/// Section 3.3.6.1.
ocudu::fapi::error_indication build_valid_error_indication();

/// Builds and returns a valid OUT_OF_SYNC ERROR.indication message. Every parameter is within the range defined in
/// SCF-222 v4.0 Section 3.3.6.1.
ocudu::fapi::error_indication build_valid_out_of_sync_error_indication();

/// Builds and returns a valid MSG_INVALID_SFN ERROR.indication message. Every parameter is within the range defined in
/// SCF-222 v4.0 Section 3.3.6.1.
ocudu::fapi::error_indication build_valid_invalid_sfn_error_indication();

/// Builds and returns a valid MSG_SLOT_ERR ERROR.indication message. Every parameter is within the range defined in
/// SCF-222 v4.0 Section 3.3.6.1.
ocudu::fapi::error_indication build_valid_msg_error_indication();

/// Builds and returns a valid MSG_TX_ERR ERROR.indication message. Every parameter is within the range defined in
/// SCF-222 v4.0 Section 3.3.6.1.
ocudu::fapi::error_indication build_valid_tx_err_error_indication();

/// Builds and returns a valid MSG_UL_DCI_ERR ERROR.indication message. Every parameter is within the range defined in
/// SCF-222 v4.0 Section 3.3.6.1.
ocudu::fapi::error_indication build_valid_ul_dci_err_error_indication();

/// Builds and returns a RACH.indication message. Every parameter is within the range defined in SCF-222 v4.0
/// Section 3.4.11.
ocudu::fapi::rach_indication build_valid_rach_indication();

/// Builds and returns a valid Rx_Data.indication message. Every parameter is within the range defined in SCF-222 v4.0
/// Section 3.4.7.
ocudu::fapi::rx_data_indication build_valid_rx_data_indication();

/// Builds and returns a valid SLOT.indication message. Every parameter is within the range defined in SCF-222 v4.0
/// Section 3.4.1.
ocudu::fapi::slot_indication build_valid_slot_indication();

/// Builds and returns a valid Tx_Data.request message. Every parameter is within the range defined in SCF-222 v4.0
/// Section 3.4.6.
/// \note Payload pointer is set to \c nullptr.
ocudu::fapi::tx_data_request build_valid_tx_data_request();

/// Builds and returns a valid transmission precoding and beamforming PDU. Every parameter is within the range defined
/// in SCF-222 v4.0 Section 3.4.2.5.
ocudu::fapi::tx_precoding_and_beamforming_pdu build_valid_tx_precoding_and_beamforming_pdu();

/// Builds and returns a valid UCI.indication message. Every parameter is within the range defined in SCF-222 v4.0
/// Section 3.4.9.
ocudu::fapi::uci_indication build_valid_uci_indication();

/// Builds and returns a valid UCI PUSCH PDU. Every parameter is within the range defined in SCF-222 v4.0
/// Section 3.4.9.1.
ocudu::fapi::uci_pusch_pdu build_valid_uci_pusch_pdu();

/// Builds and returns a valid UCI PUCCH format 0/1 PDU. Every parameter is within the range defined in SCF-222 v4.0
/// Section 3.4.9.2.
ocudu::fapi::uci_pucch_pdu_format_0_1 build_valid_uci_pucch_format01_pdu();

/// Builds and returns a valid UCI PUCCH format 2/3/4 PDU. Every parameter is within the range defined in SCF-222 v4.0
/// Section 3.4.9.3.
ocudu::fapi::uci_pucch_pdu_format_2_3_4 build_valid_uci_pucch_format234_pdu();

/// Builds and returns a valid UL_DCI.request message. Every parameter is within the range defined in SCF-222 v4.0
/// Section 3.4.4.
ocudu::fapi::ul_dci_request build_valid_ul_dci_request();

/// Builds and returns a valid UL PRACH PDU. Every parameter is within the range defined in SCF-222 v4.0
/// Section 3.4.3.1.
ocudu::fapi::ul_prach_pdu build_valid_ul_prach_pdu();

/// Builds and returns a valid UL PUCCH format 0 PDU. Every parameter is within the range defined in SCF-222 v4.0
/// Section 3.4.3.3.
ocudu::fapi::ul_pucch_pdu build_valid_ul_pucch_f0_pdu();

/// Builds and returns a valid UL PUCCH format 1 PDU. Every parameter is within the range defined in SCF-222 v4.0
/// Section 3.4.3.3.
ocudu::fapi::ul_pucch_pdu build_valid_ul_pucch_f1_pdu();

/// Builds and returns a valid UL PUCCH format 2 PDU. Every parameter is within the range defined in SCF-222 v4.0
/// Section 3.4.3.3.
ocudu::fapi::ul_pucch_pdu build_valid_ul_pucch_f2_pdu();

/// Builds and returns a valid UL PUCCH format 3 PDU. Every parameter is within the range defined in SCF-222 v4.0
/// Section 3.4.3.3.
ocudu::fapi::ul_pucch_pdu build_valid_ul_pucch_f3_pdu();

/// Builds and returns a valid UL PUCCH format 4 PDU. Every parameter is within the range defined in SCF-222 v4.0
/// Section 3.4.3.3.
ocudu::fapi::ul_pucch_pdu build_valid_ul_pucch_f4_pdu();

/// Builds and returns a valid UL PUSCH PDU. Every parameter is within the range defined in SCF-222 v4.0
/// Section 3.4.3.2.
ocudu::fapi::ul_pusch_pdu build_valid_ul_pusch_pdu();

/// Builds and returns a valid UL SRS PDU. Every parameter is within the range defined in SCF-222 v4.0
/// Section 3.4.3.3.
ocudu::fapi::ul_srs_pdu build_valid_ul_srs_pdu();

/// Builds and returns a valid UL_TTI.request. Every parameter is within the range defined in SCF-222 v4.0
/// Section 3.4.3.
ocudu::fapi::ul_tti_request build_valid_ul_tti_request();

/// Builds and returns a valid SRS.indication. Every parameter is within the range defined in SCF-222 v4.0
/// Section 3.4.10.
ocudu::fapi::srs_indication build_valid_srs_indication();

} // namespace unittest
