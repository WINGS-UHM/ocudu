// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#include "csi_report_on_puxch_helpers.h"
#include "ocudu/adt/interval.h"
#include "ocudu/ran/csi_report/csi_report_on_puxch_utils.h"
#include "ocudu/support/error_handling.h"

using namespace ocudu;

namespace {

/// Collects PMI sizes.
struct csi_report_typeI_single_panel_pmi_sizes {
  unsigned i_1_1;
  unsigned i_1_2;
  unsigned i_1_3;
  unsigned i_2;
};

} // namespace

/// Gets PMI sizes for TypeI-SinglePanel, Mode 1 codebook configuration as per TS38.212 Table 6.3.1.1.2-1.
static csi_report_typeI_single_panel_pmi_sizes
csi_report_get_pmi_sizes_typeI_single_panel_mode1(const pmi_codebook_single_panel_info& panel_info,
                                                  csi_report_data::ri_type              ri)
{
  unsigned nof_csi_rs_antenna_ports = 2 * panel_info.n1 * panel_info.n2;
  unsigned N1                       = panel_info.n1;
  unsigned N2                       = panel_info.n2;
  unsigned O1                       = panel_info.o1;
  unsigned O2                       = panel_info.o2;

  if ((ri == 1) && (nof_csi_rs_antenna_ports > 2) && (N2 == 1)) {
    return {log2_ceil(N1 * O1), log2_ceil(N2 * O2), 0, 2};
  }

  if ((ri == 2) && (nof_csi_rs_antenna_ports == 4) && (N2 == 1)) {
    return {log2_ceil(N1 * O1), log2_ceil(N2 * O2), 1, 1};
  }

  if ((ri == 2) && (nof_csi_rs_antenna_ports > 4) && (N2 == 1)) {
    return {log2_ceil(N1 * O1), log2_ceil(N2 * O2), 2, 1};
  }

  if (((ri == 3) || (ri == 4)) && (nof_csi_rs_antenna_ports == 4)) {
    return {log2_ceil(N1 * O1), log2_ceil(N2 * O2), 0, 1};
  }

  report_error("Unhandled case with ri={} nof_csi_rs_antenna_ports={} N2={}.", ri, nof_csi_rs_antenna_ports, N2);
}

namespace {

/// Calculates the RI/LI/CQI/CRI fields bit-widths for each codebook type.
struct ri_li_cqi_cri_size_calculator {
  /// Rank indicator restriction.
  const ri_restriction_type& ri_restriction;
  /// Input rank indicator.
  const csi_report_data::ri_type& ri;
  /// Number of configured CSI-RS resources.
  unsigned nof_csi_rs_resources;

  /// The default codebook reports an error.
  ri_li_cqi_cri_sizes operator()(std::monostate) const
  {
    report_error("Failed to get codebook RI/LI/CRI sizes: invalid codebook configuration.");
    return {};
  }

  /// Single-antenna port reports only contain wideband CQI, differential subband CQI, and CSI Resource indicator.
  ri_li_cqi_cri_sizes operator()(pmi_codebook_one_port) const
  {
    return {.ri                         = 0,
            .li                         = 0,
            .wideband_cqi_first_tb      = 4,
            .wideband_cqi_second_tb     = 0,
            .subband_diff_cqi_first_tb  = 2,
            .subband_diff_cqi_second_tb = 0,
            .cri                        = log2_ceil(nof_csi_rs_resources)};
  }

  /// Calculates the field bit-widths for the two-antenna port configuration.
  ri_li_cqi_cri_sizes operator()(pmi_codebook_two_port) const
  {
    unsigned ri_uint              = ri.value();
    unsigned ri_restriction_count = static_cast<unsigned>(ri_restriction.count());

    return {.ri                         = std::min(1U, log2_ceil(ri_restriction_count)),
            .li                         = log2_ceil(ri_uint),
            .wideband_cqi_first_tb      = 4,
            .wideband_cqi_second_tb     = 0,
            .subband_diff_cqi_first_tb  = 2,
            .subband_diff_cqi_second_tb = 0,
            .cri                        = log2_ceil(nof_csi_rs_resources)};
  }

  /// Calculates the field bit-widths for Type I Single-panel codebooks.
  ri_li_cqi_cri_sizes operator()(const pmi_codebook_typeI_single_panel& pmi_codebook) const
  {
    unsigned nof_csi_antenna_ports = csi_report_get_nof_csi_rs_antenna_ports(pmi_codebook);
    ocudu_assert(nof_csi_antenna_ports == 4, "Only four ports are currently supported.");

    unsigned ri_uint              = ri.value();
    unsigned ri_restriction_count = static_cast<unsigned>(ri_restriction.count());

    ocudu_assert(ri_restriction.find_lowest(true) >= 0,
                 "The RI restriction field (i.e., {}) must have at least one true value.",
                 ri_restriction);

    ri_li_cqi_cri_sizes result;
    result.ri =
        (nof_csi_antenna_ports == 4) ? std::min(2U, log2_ceil(ri_restriction_count)) : log2_ceil(ri_restriction_count);
    result.li                         = std::min(2U, log2_ceil(ri_uint));
    result.wideband_cqi_first_tb      = 4;
    result.wideband_cqi_second_tb     = (ri.value() > 4) ? 4 : 0;
    result.subband_diff_cqi_first_tb  = 2;
    result.subband_diff_cqi_second_tb = (ri.value() > 4) ? 2 : 0;
    result.cri                        = log2_ceil(nof_csi_rs_resources);

    return result;
  }
};

/// Calculates the total bit-width of the Precoding Matrix Indicator (PMI) fields.
struct pmi_size_calculator {
  /// Input Rank Indicator (RI).
  csi_report_data::ri_type ri;

  /// The PMI fields bit-width depends on the reported Rank Indicator (RI).
  pmi_size_calculator(csi_report_data::ri_type ri_) : ri(ri_) {}

  /// The default codebook reports an error.
  unsigned operator()(const std::monostate&) const
  {
    report_error("Failed to calculate PMI size: invalid codebook configuration.");
    return {};
  }

  /// Single-antenna port reports do not contain PMI.
  unsigned operator()(const pmi_codebook_one_port&) const { return 0; }

  /// Selects the PMI codebook size for the two-antenna port configuration.
  unsigned operator()(const pmi_codebook_two_port&) const
  {
    ocudu_assert(ri <= 2, "Invalid rank indicator (i.e., {}).", ri);
    if (ri == 2) {
      return 1;
    }

    return 2;
  }

  /// Calculates the PMI field size for Type I Single-panel codebooks.
  unsigned operator()(const pmi_codebook_typeI_single_panel& codebook) const
  {
    ocudu_assert(codebook.mode == pmi_codebook_typeI_mode::one, "Only mode 1 is currently supported.");

    unsigned count = 0;

    csi_report_typeI_single_panel_pmi_sizes sizes =
        csi_report_get_pmi_sizes_typeI_single_panel_mode1(get_single_panel_info(codebook.n1_n2), ri);

    count += sizes.i_1_1;
    count += sizes.i_1_2;
    count += sizes.i_1_3;
    count += sizes.i_2;

    return count;
  }
};

/// Precoding Matrix Indicator (PMI) unpacking helper.
struct pmi_unpacker {
  /// Reference to the packed CSI Report.
  const csi_report_packed& packed;
  /// Unpacked Rank Indicator (RI).
  csi_report_data::ri_type ri;

  /// The unpacker requires the input packed bits and the Rank Indicator (RI).
  pmi_unpacker(const csi_report_packed& packed_, csi_report_data::ri_type ri_) : packed(packed_), ri(ri_) {}

  /// The default codebook reports an error.
  csi_report_pmi operator()(const std::monostate&) const
  {
    report_error("Failed to unpack PMI: invalid codebook configuration.");
    return {};
  }

  /// PMI is not present for single-antenna port.
  csi_report_pmi operator()(const pmi_codebook_one_port&) const { return {}; }

  /// Unpack the PMI for two antenna port configuration.
  csi_report_pmi operator()(const pmi_codebook_two_port&) const
  {
    csi_report_pmi::two_antenna_port result;
    result.pmi = packed.extract(0, packed.size());
    return csi_report_pmi{result};
  }

  /// Unpack the PMI for Type I Single-panel codebook configuration.
  csi_report_pmi operator()(const pmi_codebook_typeI_single_panel& codebook) const
  {
    ocudu_assert(codebook.mode == pmi_codebook_typeI_mode::one, "Only mode 1 is currently supported.");

    unsigned                                        count = 0;
    csi_report_pmi::typeI_single_panel_4ports_mode1 result;
    csi_report_typeI_single_panel_pmi_sizes         sizes =
        csi_report_get_pmi_sizes_typeI_single_panel_mode1(get_single_panel_info(codebook.n1_n2), ri);

    result.i_1_1 = packed.extract(count, sizes.i_1_1);
    count += sizes.i_1_1;
    ocudu_assert(sizes.i_1_2 == 0, "PMI field i_1_2 size must be 0 bits for 4 ports.");

    if (ri > 1) {
      result.i_1_3.emplace(packed.extract(count, sizes.i_1_3));
      count += sizes.i_1_3;
    }

    result.i_2 = packed.extract(count, sizes.i_2);
    count += sizes.i_2;

    ocudu_assert(packed.size() == count,
                 "Packet input size (i.e., {}) does not match with the fields size (i.e., {})",
                 packed.size(),
                 count);
    return csi_report_pmi{result};
  }
};

} // namespace

ri_li_cqi_cri_sizes ocudu::get_ri_li_cqi_cri_sizes(const pmi_codebook_config& pmi_codebook,
                                                   const ri_restriction_type& ri_restriction,
                                                   csi_report_data::ri_type   ri,
                                                   unsigned                   nof_csi_rs_resources)
{
  // Calculate CRI field size. The number of CSI resources in the corresponding resource set must be at least one and up
  // to 64 (see TS38.331 Section 6.3.2, Information Element \c NZP-CSI-RS-ResourceSet).
  constexpr interval<unsigned, true> nof_csi_res_range(1, 64);
  ocudu_assert(nof_csi_res_range.contains(nof_csi_rs_resources),
               "The number of CSI-RS resources in the resource set, i.e., {} exceeds the valid range {}.",
               nof_csi_rs_resources,
               nof_csi_res_range);

  return std::visit(ri_li_cqi_cri_size_calculator{ri_restriction, ri, nof_csi_rs_resources}, pmi_codebook);
}

unsigned ocudu::csi_report_get_size_pmi(const pmi_codebook_config& codebook, csi_report_data::ri_type ri)
{
  return std::visit(pmi_size_calculator{ri}, codebook);
}

csi_report_data::wideband_cqi_type ocudu::csi_report_unpack_wideband_cqi(csi_report_packed packed)
{
  ocudu_assert(packed.size() == 4, "Packed size (i.e., {}) must be 4 bits.", packed.size());
  return packed.extract(0, 4);
}

csi_report_pmi ocudu::csi_report_unpack_pmi(const csi_report_packed&   packed,
                                            const pmi_codebook_config& codebook,
                                            csi_report_data::ri_type   ri)
{
  return std::visit(pmi_unpacker{packed, ri}, codebook);
}

csi_report_data::ri_type ocudu::csi_report_unpack_ri(const csi_report_packed&   ri_packed,
                                                     const ri_restriction_type& ri_restriction)
{
  unsigned ri = 1;
  if (!ri_packed.empty()) {
    ri = ri_packed.extract(0, ri_packed.size());

    ocudu_assert(ri < ri_restriction.count(),
                 "Packed RI, i.e., {}, is out of bounds given the number of allowed rank values, i.e., {}.",
                 ri,
                 ri_restriction.count());

    ri = ri_restriction.get_bit_positions()[ri] + 1;
  }
  return ri;
}
