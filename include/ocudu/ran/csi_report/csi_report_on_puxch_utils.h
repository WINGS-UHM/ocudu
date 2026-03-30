// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#pragma once

#include "ocudu/ran/csi_report/csi_report_configuration.h"

namespace ocudu {

/// Gets the number of CSI-RS antenna ports from the PMI codebook type.
inline unsigned csi_report_get_nof_csi_rs_antenna_ports(const pmi_codebook_config& pmi_codebook)
{
  struct overloaded {
    unsigned operator()(std::monostate) const { return 0; }
    unsigned operator()(pmi_codebook_one_port) const { return 1; }
    unsigned operator()(pmi_codebook_two_port) const { return 2; }
    unsigned operator()(const pmi_codebook_typeI_single_panel& codebook) const
    {
      pmi_codebook_single_panel_info panel_config = get_single_panel_info(codebook.n1_n2);
      return 2 * panel_config.n1 * panel_config.n2;
    }
  };

  return std::visit(overloaded{}, pmi_codebook);
}

} // namespace ocudu
