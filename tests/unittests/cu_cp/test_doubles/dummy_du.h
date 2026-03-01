// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#pragma once

#include <memory>

namespace ocudu {

struct f1ap_message;

namespace ocucp {

class cu_cp_f1c_handler;

class du_test_stub
{
public:
  virtual ~du_test_stub() = default;

  virtual void push_tx_pdu(const f1ap_message& msg) = 0;

  virtual bool try_pop_rx_pdu(f1ap_message& msg) = 0;
};

struct du_stub_params {
  cu_cp_f1c_handler& cu_cp_f1c_itf;
};

/// Creates an emulator of a DU from the perspective of the CU-CP.
std::unique_ptr<du_test_stub> create_du_stub(du_stub_params params);

} // namespace ocucp
} // namespace ocudu
