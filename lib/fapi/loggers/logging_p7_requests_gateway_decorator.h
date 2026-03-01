// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI

#pragma once

#include "ocudu/fapi/p7/p7_requests_gateway.h"
#include "ocudu/ocudulog/ocudulog.h"

namespace ocudu {
namespace fapi {

/// Adds logging information over the implemented interface.
class logging_p7_requests_gateway_decorator : public p7_requests_gateway
{
public:
  logging_p7_requests_gateway_decorator(unsigned                sector_id_,
                                        ocudulog::basic_logger& logger_,
                                        p7_requests_gateway&    p7_gateway_) :
    sector_id(sector_id_), logger(logger_), p7_gateway(p7_gateway_)
  {
  }

  // See interface for documentation.
  void send_dl_tti_request(const dl_tti_request& msg) override;

  // See interface for documentation.
  void send_ul_tti_request(const ul_tti_request& msg) override;

  // See interface for documentation.
  void send_ul_dci_request(const ul_dci_request& msg) override;

  // See interface for documentation.
  void send_tx_data_request(const tx_data_request& msg) override;

private:
  const unsigned          sector_id;
  ocudulog::basic_logger& logger;
  p7_requests_gateway&    p7_gateway;
};

} // namespace fapi
} // namespace ocudu
