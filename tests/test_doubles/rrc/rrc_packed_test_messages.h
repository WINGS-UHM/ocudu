// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#pragma once

/// \file
/// \brief Generation of packed RRC messages for testing purposes. Use this file when you don't want to include
/// the RRC ASN.1 headers.

#include "ocudu/adt/byte_buffer.h"
#include "ocudu/ran/plmn_identity.h"
#include "ocudu/ran/subcarrier_spacing.h"

namespace ocudu {
namespace test_helpers {

/// Generates a dummy RRC handoverPrepInformation as per TS 38.331.
byte_buffer create_ho_prep_info();

/// \brief Generates a dummy Measurement Timing Configuration.
byte_buffer create_meas_timing_cfg(uint32_t carrier_freq, subcarrier_spacing scs);

/// \brief Generates a packed dummy SIB1 message.
byte_buffer create_packed_sib1(const plmn_identity& plmn = plmn_identity::test_value());

/// \brief Generates a dummy SIB1 hex string.
std::string create_sib1_hex_string(const plmn_identity& plmn = plmn_identity::test_value());

/// \brief Generates a dummy CellGroupConfig.
byte_buffer create_cell_group_config();

} // namespace test_helpers
} // namespace ocudu
