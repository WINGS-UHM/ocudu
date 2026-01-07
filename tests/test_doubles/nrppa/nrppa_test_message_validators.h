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

#include "ocudu/asn1/nrppa/nrppa.h"

namespace ocudu::test_helpers {

bool is_valid_e_cid_meas_initiation_failure(const asn1::nrppa::nr_ppa_pdu_c& msg);

bool is_valid_e_cid_meas_initiation_response(const asn1::nrppa::nr_ppa_pdu_c& msg);

bool is_valid_e_cid_meas_report(const asn1::nrppa::nr_ppa_pdu_c& msg);

bool is_valid_nrppa_trp_information_response(const asn1::nrppa::nr_ppa_pdu_c& msg);

bool is_valid_nrppa_trp_information_failure(const asn1::nrppa::nr_ppa_pdu_c& msg);

bool is_valid_nrppa_positioning_information_response(const asn1::nrppa::nr_ppa_pdu_c& msg);

bool is_valid_nrppa_positioning_information_failure(const asn1::nrppa::nr_ppa_pdu_c& msg);

bool is_valid_nrppa_positioning_activation_response(const asn1::nrppa::nr_ppa_pdu_c& msg);

bool is_valid_nrppa_positioning_activation_failure(const asn1::nrppa::nr_ppa_pdu_c& msg);

bool is_valid_nrppa_measurement_response(const asn1::nrppa::nr_ppa_pdu_c& msg);

bool is_valid_nrppa_measurement_failure(const asn1::nrppa::nr_ppa_pdu_c& msg);

} // namespace ocudu::test_helpers
