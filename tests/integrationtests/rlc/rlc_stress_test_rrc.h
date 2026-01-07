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

#include "lib/rlc/rlc_bearer_logger.h"
#include "ocudu/pdcp/pdcp_rx.h"
#include "ocudu/pdcp/pdcp_tx.h"

namespace ocudu {
class rrc_dummy final : public pdcp_rx_upper_control_notifier, public pdcp_tx_upper_control_notifier
{
  rlc_bearer_logger logger;

public:
  explicit rrc_dummy(uint32_t id) : logger("RRC", {gnb_du_id_t::min, id, drb_id_t::drb1, "DL/UL"}) {}

  // PDCP -> RRC
  void on_integrity_failure() override {}
  void on_protocol_failure() override {}
  void on_max_count_reached() override {}
  void on_resume_required() override {}
};

} // namespace ocudu
