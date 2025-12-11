/*
 *
 * Copyright 2021-2025 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "pdcp_tx_suspend_test.h"
#include "pdcp_test_vectors.h"
#include "ocudu/pdcp/pdcp_config.h"
#include "ocudu/support/test_utils.h"
#include <gtest/gtest.h>

using namespace ocudu;

/// Test DRB suspend.
TEST_P(pdcp_tx_suspend_test, when_suspend_called_state_is_reset)
{
  init(GetParam(), pdcp_rb_type::drb);

  // Set state of PDCP entity.
  pdcp_tx_state st = {0, 0, 0, 0, 0};
  pdcp_tx->set_state(st);
  pdcp_tx->configure_security(sec_cfg, security::integrity_enabled::on, security::ciphering_enabled::off);

  // Write 5 SDUs.
  int n_pdus = 5;
  for (int i = 0; i < n_pdus; i++) {
    byte_buffer sdu = byte_buffer::create(sdu1).value();
    pdcp_tx->handle_sdu(std::move(sdu));
  }
  wait_pending_crypto();
  worker.run_pending_tasks();

  FLUSH_AND_ASSERT_EQ(5, pdcp_tx->nof_pdus_in_window());
  pdcp_tx->begin_buffering();
  pdcp_tx->suspend();
  FLUSH_AND_ASSERT_EQ(0, pdcp_tx->nof_pdus_in_window());

  // Check the state is reset.
  pdcp_tx_state exp_st{0, 0, 0, 0, 0};
  assert_pdcp_state_with_reordering(pdcp_tx->get_state(), exp_st);

  // Write 5 SDUs again. These should be buffered.
  // Check for not PDUs in the window and only one resume request.
  for (int i = 0; i < n_pdus; i++) {
    byte_buffer sdu = byte_buffer::create(sdu1).value();
    pdcp_tx->handle_sdu(std::move(sdu));
  }
  FLUSH_AND_ASSERT_EQ(0, pdcp_tx->nof_pdus_in_window());
  FLUSH_AND_ASSERT_EQ(1, test_frame.nof_resume_required);

  // Resume PDCP entity. Buffered SDUs should be flushed.
  pdcp_tx->resume();
  pdcp_tx->end_buffering();

  FLUSH_AND_ASSERT_EQ(5, pdcp_tx->nof_pdus_in_window());
  FLUSH_AND_ASSERT_EQ(1, test_frame.nof_resume_required);
}

///////////////////////////////////////////////////////////////////
// Finally, instantiate all testcases for each supported SN size //
///////////////////////////////////////////////////////////////////
std::string test_param_info_to_string(const ::testing::TestParamInfo<std::tuple<pdcp_sn_size, unsigned>>& info)
{
  fmt::memory_buffer buffer;
  fmt::format_to(std::back_inserter(buffer), "{}bit", pdcp_sn_size_to_uint(std::get<pdcp_sn_size>(info.param)));
  return fmt::to_string(buffer);
}

INSTANTIATE_TEST_SUITE_P(pdcp_tx_test_all_sn_sizes,
                         pdcp_tx_suspend_test,
                         ::testing::Combine(::testing::Values(pdcp_sn_size::size12bits, pdcp_sn_size::size18bits),
                                            ::testing::Values(1)),
                         test_param_info_to_string);

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
