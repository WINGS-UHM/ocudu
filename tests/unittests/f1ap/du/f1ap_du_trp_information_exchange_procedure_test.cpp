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
#include <gtest/gtest.h>

using namespace ocudu;
using namespace odu;

class f1ap_du_trp_information_exchange_procedure_test : public f1ap_du_test
{
protected:
  f1ap_du_trp_information_exchange_procedure_test()
  {
    // Test Preamble.
    run_f1_setup_procedure();
    this->f1c_gw.clear_tx_pdus();
  }
};

TEST_F(f1ap_du_trp_information_exchange_procedure_test, when_trp_info_exchange_succeeds_then_response_is_sent_to_cu)
{
  f1ap_message req = test_helpers::generate_trp_information_request();

  this->f1ap->handle_message(req);

  auto tx_msg = this->f1c_gw.pop_tx_pdu();
  ASSERT_TRUE(test_helpers::is_valid_f1ap_trp_information_response(tx_msg.value()));
  auto& resp = tx_msg.value().pdu.successful_outcome().value.trp_info_resp();
  ASSERT_EQ(resp->trp_info_list_trp_resp.size(), 3);
}
