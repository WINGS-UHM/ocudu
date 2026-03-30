// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#pragma once

#include "ocudu/ran/csi_report/csi_report_configuration.h"
#include "ocudu/ran/csi_report/csi_report_data.h"
#include "ocudu/ran/csi_report/csi_report_packed.h"
#include "ocudu/ran/precoding/precoding_codebook_configuration.h"

namespace ocudu {

/// Collects the RI, LI, wideband CQI, and CSI fields bit-widths.
struct ri_li_cqi_cri_sizes {
  unsigned ri;
  unsigned li;
  unsigned wideband_cqi_first_tb;
  unsigned wideband_cqi_second_tb;
  unsigned subband_diff_cqi_first_tb;
  unsigned subband_diff_cqi_second_tb;
  unsigned cri;
};

/// Gets the bit-widths of the RI, LI, wideband CQI, and CRI fields.
ri_li_cqi_cri_sizes get_ri_li_cqi_cri_sizes(const pmi_codebook_config& pmi_codebook,
                                            const ri_restriction_type& ri_restriction,
                                            csi_report_data::ri_type   ri,
                                            unsigned                   nof_csi_rs_resources);

/// Gets the PMI field bit-width.
unsigned csi_report_get_size_pmi(const pmi_codebook_config& codebook, csi_report_data::ri_type ri);

/// Unpacks the wideband CQI field from a packed CSI report.
csi_report_data::wideband_cqi_type csi_report_unpack_wideband_cqi(csi_report_packed packed);

/// Unpacks the PMI fields from a packed CSI report.
csi_report_pmi csi_report_unpack_pmi(const csi_report_packed&   packed,
                                     const pmi_codebook_config& codebook,
                                     csi_report_data::ri_type   ri);

/// Unpacks RI as per TS38.212 Section 6.3.1.1.2. and TS38.214 Section 5.2.2.2.1.
csi_report_data::ri_type csi_report_unpack_ri(const csi_report_packed&   ri_packed,
                                              const ri_restriction_type& ri_restriction);

} // namespace ocudu
