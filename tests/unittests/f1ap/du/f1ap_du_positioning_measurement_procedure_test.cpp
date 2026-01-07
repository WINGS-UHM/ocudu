/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "f1ap_du_test_helpers.h"
#include "tests/test_doubles/f1ap/f1ap_test_message_validators.h"
#include "tests/test_doubles/f1ap/f1ap_test_messages.h"
#include "ocudu/asn1/f1ap/common.h"
#include "ocudu/asn1/f1ap/f1ap_pdu_contents.h"
#include "ocudu/du/du_cell_config_helpers.h"
#include <gtest/gtest.h>

using namespace ocudu;
using namespace odu;

class f1ap_du_positioning_measurement_procedure_test : public f1ap_du_test
{
protected:
  f1ap_du_positioning_measurement_procedure_test()
  {
    // Test Preamble.
    run_f1_setup_procedure();
    run_f1ap_ue_create(test_ue_index);
    f1ap_message msg = test_helpers::generate_ue_context_setup_request(
        gnb_cu_ue_f1ap_id_t{0}, gnb_du_ue_f1ap_id_t{0}, 1, {}, config_helpers::make_default_du_cell_config().nr_cgi);
    run_ue_context_setup_procedure(test_ue_index, msg);

    this->f1c_gw.clear_tx_pdus();
  }

  du_ue_index_t test_ue_index = to_du_ue_index(0);
};

TEST_F(f1ap_du_positioning_measurement_procedure_test, when_valid_request_is_received_then_du_is_notified)
{
  f1ap_message req =
      test_helpers::generate_positioning_measurement_request({trp_id_t::min}, lmf_meas_id_t::min, ran_meas_id_t::min);

  this->f1ap->handle_message(req);

  ASSERT_TRUE(this->f1ap_du_cfg_handler.last_positioning_meas_request.has_value());
}

TEST_F(f1ap_du_positioning_measurement_procedure_test, when_invalid_request_is_received_then_failure_is_sent_to_cu)
{
  f1ap_message req =
      test_helpers::generate_positioning_measurement_request({trp_id_t::min},
                                                             lmf_meas_id_t::min,
                                                             ran_meas_id_t::min,
                                                             {asn1::f1ap::pos_meas_type_opts::options::ul_rtoa,
                                                              asn1::f1ap::pos_meas_type_opts::options::ul_srs_rsrp,
                                                              asn1::f1ap::pos_meas_type_opts::options::ul_aoa});

  // Remove SRS Configuration from the request to simulate an invalid request.
  req.pdu.init_msg().value.positioning_meas_request()->srs_configuration_present = false;

  this->f1ap->handle_message(req);

  // DU is not notified about the invalid request.
  ASSERT_FALSE(this->f1ap_du_cfg_handler.last_positioning_meas_request.has_value());

  auto tx_msg = this->f1c_gw.pop_tx_pdu();
  ASSERT_TRUE(test_helpers::is_valid_f1ap_positioning_measurement_failure(tx_msg.value()));
}

TEST_F(f1ap_du_positioning_measurement_procedure_test,
       when_positioning_measurement_succeeds_then_response_is_sent_to_cu)
{
  f1ap_message req =
      test_helpers::generate_positioning_measurement_request({trp_id_t::min}, lmf_meas_id_t::min, ran_meas_id_t::min);

  this->f1ap->handle_message(req);

  auto tx_msg = this->f1c_gw.pop_tx_pdu();
  ASSERT_TRUE(test_helpers::is_valid_f1ap_positioning_measurement_response(tx_msg.value()));
}
