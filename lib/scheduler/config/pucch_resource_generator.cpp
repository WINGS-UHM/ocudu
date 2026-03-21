// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#include "ocudu/scheduler/config/pucch_resource_generator.h"
#include "ocudu/adt/expected.h"
#include "ocudu/ran/pucch/pucch_info.h"
#include "ocudu/ran/pucch/pucch_mapping.h"
#include "ocudu/ran/resource_allocation/ofdm_symbol_range.h"
#include "ocudu/ran/resource_allocation/rb_interval.h"
#include "ocudu/scheduler/config/pucch_resource_builder_params.h"
#include "ocudu/support/math/math_utils.h"
#include "fmt/ranges.h"
#include "fmt/std.h"
#include <algorithm>
#include <variant>

using namespace ocudu;
using namespace config_helpers;

namespace {

struct pucch_grant {
  pucch_format                format;
  ofdm_symbol_range           symbols;
  prb_interval                prbs;
  std::optional<prb_interval> freq_hop_grant;
  std::optional<unsigned>     occ_cs_idx;
};

} // namespace

/// Given the OCC-CS index (implementation-defined), maps and returns the \c initialCyclicShift, defined as per
/// PUCCH-format1, in PUCCH-Config, TS 38.331.
static unsigned occ_cs_index_to_cyclic_shift(unsigned occ_cs_idx, unsigned nof_css)
{
  // NOTE: the OCC-CS index -> (CS, OCC) mapping is defined as follows.
  // i)   Define CS_step = 12 / nof_css. NOTE that 12 is divisible by nof_css.
  // ii)  occ_cs_idx = 0, 1, 2, ...  => (CS = 0, OCC=0), (CS = CS_step, OCC=0), (CS = 2*CS_step, OCC=0), ...
  // iii) occ_cs_idx = nof_css, nof_css+1, ...  => (CS = 0, OCC=1), (CS = CS_step, OCC=1), ...
  // iv)  occ_cs_idx = 2*nof_css, (2*nof_css)+1, ...  => (CS = 0, OCC=2), (CS = CS_step, OCC=2), ...
  const unsigned max_nof_css = format1_cp_step_to_uint(pucch_nof_cyclic_shifts::twelve);
  const unsigned cs_step     = max_nof_css / nof_css;
  return (occ_cs_idx * cs_step) % max_nof_css;
}

/// Given the OCC-CS index (implementation-defined), maps and returns the \c timeDomainOCC, defined as per
/// PUCCH-format1, in PUCCH-Config, TS 38.331.
static unsigned occ_cs_index_to_occ(unsigned occ_cs_idx, unsigned nof_css)
{
  // NOTE: the OCC-CS index -> (CS, OCC) mapping is defined as follows.
  // i)   Define CS_step = 12 / nof_css. NOTE that 12 is divisible by nof_css.
  // ii)  occ_cs_idx = 0, 1, 2, ...  => (CS = 0, OCC=0), (CS = CS_step, OCC=0), (CS = 2*CS_step, OCC=0), ...
  // iii) occ_cs_idx = nof_css, nof_css+1, ...  => (CS = 0, OCC=1), (CS = CS_step, OCC=1), ...
  // iv)  occ_cs_idx = 2*nof_css, (2*nof_css)+1, ...  => (CS = 0, OCC=2), (CS = CS_step, OCC=2), ...
  return occ_cs_idx / nof_css;
}

/// Given the OCC-CS index (implementation-defined), maps and returns the \c OCC-Index, defined as per
/// PUCCH-format4, in PUCCH-Config, TS 38.331.
static pucch_f4_occ_idx occ_cs_index_to_f4_occ(unsigned occ_cs_idx)
{
  return static_cast<pucch_f4_occ_idx>(occ_cs_idx);
}

static std::vector<pucch_grant> compute_f0_res(unsigned                         nof_res_f0,
                                               pucch_f0_params                  params,
                                               unsigned                         bwp_size_rbs,
                                               bounded_integer<unsigned, 1, 14> max_nof_symbols)
{
  // Compute the number of symbols and RBs for F0.
  std::vector<pucch_grant>  res_list;
  const unsigned            nof_syms_f0 = params.nof_syms.value();
  static constexpr unsigned f0_max_rbs  = 1U;

  // With intraslot freq. hopping.
  if (params.intraslot_freq_hopping) {
    for (unsigned rb_idx = 0, rb_stop = bwp_size_rbs / 2 - f0_max_rbs; rb_idx != rb_stop; ++rb_idx) {
      // Generate resource for increasing RB index, until the num. of required resources is reached.
      const prb_interval prbs{rb_idx, rb_idx + f0_max_rbs};
      // Pre-compute the second RBs for the frequency hop specularly to the first RBs.
      const prb_interval freq_hop_prbs{bwp_size_rbs - f0_max_rbs - rb_idx, bwp_size_rbs - rb_idx};

      // Generate resource for increasing Symbol index, until the num. of required resources is reached.
      for (unsigned sym_idx = 0; sym_idx + nof_syms_f0 <= max_nof_symbols.value(); sym_idx += nof_syms_f0) {
        const ofdm_symbol_range symbols{sym_idx, sym_idx + nof_syms_f0};

        // Allocate resources for first hop.
        res_list.emplace_back(pucch_grant{
            .format = pucch_format::FORMAT_0, .symbols = symbols, .prbs = prbs, .freq_hop_grant = freq_hop_prbs});
        if (res_list.size() == nof_res_f0) {
          break;
        }

        // Allocate resources for second hop.
        res_list.emplace_back(pucch_grant{
            .format = pucch_format::FORMAT_0, .symbols = symbols, .prbs = freq_hop_prbs, .freq_hop_grant = prbs});
        if (res_list.size() == nof_res_f0) {
          break;
        }
      }
      if (res_list.size() == nof_res_f0) {
        break;
      }
    }
  }
  // Without intraslot freq. hopping.
  else {
    for (unsigned rb_idx = 0, rb_stop = bwp_size_rbs / 2 - f0_max_rbs; rb_idx != rb_stop; ++rb_idx) {
      const prb_interval prbs_low_spectrum{rb_idx, rb_idx + f0_max_rbs};
      // Pre-compute the RBs for the resources to be allocated on the upper part of the spectrum. This is to achieve
      // balancing of the PUCCH resources on both sides of the BWP.
      const prb_interval prbs_hi_spectrum{bwp_size_rbs - f0_max_rbs - rb_idx, bwp_size_rbs - rb_idx};

      // Generate resource for increasing Symbol index, until the num. of required resources is reached.
      for (unsigned sym_idx = 0; sym_idx + nof_syms_f0 <= max_nof_symbols.value(); sym_idx += nof_syms_f0) {
        const ofdm_symbol_range symbols{sym_idx, sym_idx + nof_syms_f0};
        res_list.emplace_back(
            pucch_grant{.format = pucch_format::FORMAT_0, .symbols = symbols, .prbs = prbs_low_spectrum});
        if (res_list.size() == nof_res_f0) {
          break;
        }
      }
      if (res_list.size() == nof_res_f0) {
        break;
      }

      // Repeat the resource allocation on the upper part of the spectrum, to spread the PUCCH resource on both sides of
      // the BWP.
      for (unsigned sym_idx = 0; sym_idx + nof_syms_f0 <= NOF_OFDM_SYM_PER_SLOT_NORMAL_CP; sym_idx += nof_syms_f0) {
        const ofdm_symbol_range symbols{sym_idx, sym_idx + nof_syms_f0};
        res_list.emplace_back(
            pucch_grant{.format = pucch_format::FORMAT_0, .symbols = symbols, .prbs = prbs_hi_spectrum});
        if (res_list.size() == nof_res_f0) {
          break;
        }
      }

      if (res_list.size() == nof_res_f0) {
        break;
      }
    }
  }

  return res_list;
}

static std::vector<pucch_grant> compute_f1_res(unsigned                         nof_res_f1,
                                               pucch_f1_params                  params,
                                               unsigned                         bwp_size_rbs,
                                               unsigned                         nof_occ_css,
                                               bounded_integer<unsigned, 1, 14> max_nof_symbols)
{
  std::vector<pucch_grant>  res_list;
  static constexpr unsigned f1_max_rbs = 1U;

  // With intraslot_freq_hopping.
  if (params.intraslot_freq_hopping) {
    // Generate resource for increasing RB index, until the num. of required resources is reached.
    for (unsigned rb_idx = 0, rb_stop = bwp_size_rbs / 2 - f1_max_rbs; rb_idx != rb_stop; ++rb_idx) {
      const prb_interval prbs{rb_idx, rb_idx + f1_max_rbs};
      // Pre-compute the second RBs for the frequency hop specularly to the first RBs.
      const prb_interval freq_hop_prbs{bwp_size_rbs - f1_max_rbs - rb_idx, bwp_size_rbs - rb_idx};

      // Generate resource for increasing Symbol index, until the num. of required resources is reached.
      for (unsigned sym_idx = 0; sym_idx + params.nof_syms.value() <= max_nof_symbols.value();
           sym_idx += params.nof_syms.value()) {
        const ofdm_symbol_range symbols{sym_idx, sym_idx + params.nof_syms.value()};

        // Allocate all OCC and CSS resources for first hop, until the num. of required resources is reached.
        // NOTE: occ_cs_idx is an index that will be mapped into Initial Cyclic Shift and Orthogonal Cover Code (OCC).
        // Two PUCCH F1 resources over the same symbols and RBs are orthogonal if they have different CS or different
        // OCC; this leads to a tot. nof OCCs x tot. nof CSs possible orthogonal F1 resources over the same REs.
        for (unsigned occ_cs_idx = 0; occ_cs_idx != nof_occ_css; ++occ_cs_idx) {
          res_list.emplace_back(pucch_grant{.format         = pucch_format::FORMAT_1,
                                            .symbols        = symbols,
                                            .prbs           = prbs,
                                            .freq_hop_grant = freq_hop_prbs,
                                            .occ_cs_idx     = occ_cs_idx});
          if (res_list.size() == nof_res_f1) {
            break;
          }
        }
        if (res_list.size() == nof_res_f1) {
          break;
        }

        // Allocate all OCC and CSS resources for second hop, until the num. of required resources is reached.
        for (unsigned occ_cs_idx = 0; occ_cs_idx != nof_occ_css; ++occ_cs_idx) {
          res_list.emplace_back(pucch_grant{.format         = pucch_format::FORMAT_1,
                                            .symbols        = symbols,
                                            .prbs           = freq_hop_prbs,
                                            .freq_hop_grant = prbs,
                                            .occ_cs_idx     = occ_cs_idx});
          if (res_list.size() == nof_res_f1) {
            break;
          }
        }
        if (res_list.size() == nof_res_f1) {
          break;
        }
      }
      if (res_list.size() == nof_res_f1) {
        break;
      }
    }
  }
  // Without intraslot freq. hopping.
  else {
    // Generate resource for increasing RB index, until the num. of required resources is reached.
    for (unsigned rb_idx = 0, rb_stop = bwp_size_rbs / 2 - f1_max_rbs; rb_idx != rb_stop; ++rb_idx) {
      const prb_interval prbs_low_spectrum{rb_idx, rb_idx + f1_max_rbs};
      // Pre-compute the RBs for the resources to be allocated on the upper part of the spectrum. This is to achieve
      // balancing of the PUCCH resources on both sides of the BWP.
      const prb_interval prbs_hi_spectrum{bwp_size_rbs - f1_max_rbs - rb_idx, bwp_size_rbs - rb_idx};

      // Generate resource for increasing Symbol index, until the num. of required resources is reached.
      for (unsigned sym_idx = 0; sym_idx + params.nof_syms.value() <= max_nof_symbols.value();
           sym_idx += params.nof_syms.value()) {
        const ofdm_symbol_range symbols{sym_idx, sym_idx + params.nof_syms.value()};

        // Allocate all OCC and CS resources, until the num. of required resources is reached.
        for (unsigned occ_cs_idx = 0; occ_cs_idx != nof_occ_css; ++occ_cs_idx) {
          res_list.emplace_back(pucch_grant{.format     = pucch_format::FORMAT_1,
                                            .symbols    = symbols,
                                            .prbs       = prbs_low_spectrum,
                                            .occ_cs_idx = occ_cs_idx});
          if (res_list.size() == nof_res_f1) {
            break;
          }
        }
        if (res_list.size() == nof_res_f1) {
          break;
        }
      }
      if (res_list.size() == nof_res_f1) {
        break;
      }

      // Repeat the resource allocation on the upper part of the spectrum, to spread the PUCCH resource on both sides of
      // the BWP.
      for (unsigned sym_idx = 0; sym_idx + params.nof_syms.value() <= max_nof_symbols.value();
           sym_idx += params.nof_syms.value()) {
        const ofdm_symbol_range symbols{sym_idx, sym_idx + params.nof_syms.value()};

        for (unsigned occ_cs_idx = 0; occ_cs_idx != nof_occ_css; ++occ_cs_idx) {
          res_list.emplace_back(pucch_grant{.format     = pucch_format::FORMAT_1,
                                            .symbols    = symbols,
                                            .prbs       = prbs_hi_spectrum,
                                            .occ_cs_idx = occ_cs_idx});
          if (res_list.size() == nof_res_f1) {
            break;
          }
        }
        if (res_list.size() == nof_res_f1) {
          break;
        }
      }

      if (res_list.size() == nof_res_f1) {
        break;
      }
    }
  }

  return res_list;
}

static std::vector<pucch_grant> compute_f2_res(unsigned                         nof_res_f2,
                                               const pucch_f2_params&           params,
                                               unsigned                         bwp_size_rbs,
                                               bounded_integer<unsigned, 1, 14> max_nof_symbols)
{
  // Compute the number of symbols and RBs for F2.
  std::vector<pucch_grant> res_list;
  const unsigned           nof_f2_symbols = params.nof_syms.value();
  const unsigned           f2_max_rbs     = params.max_payload_bits.has_value()
                                                ? get_pucch_format2_max_nof_prbs(params.max_payload_bits.value(),
                                                                   nof_f2_symbols,
                                                                   to_max_code_rate_float(params.max_code_rate))
                                                : params.max_nof_rbs.value();

  if (f2_max_rbs > pucch_constants::f2::MAX_NOF_RBS) {
    return {};
  }

  // With intraslot freq. hopping.
  if (params.intraslot_freq_hopping) {
    for (unsigned rb_idx = 0, rb_stop = bwp_size_rbs / 2 - f2_max_rbs; rb_idx < rb_stop; rb_idx += f2_max_rbs) {
      // Generate resource for increasing RB index, until the num. of required resources is reached.
      const prb_interval prbs{rb_idx, rb_idx + f2_max_rbs};
      // Pre-compute the second RBs for the frequency hop specularly to the first RBs.
      const prb_interval freq_hop_prbs{bwp_size_rbs - f2_max_rbs - rb_idx, bwp_size_rbs - rb_idx};

      // Generate resource for increasing Symbol index, until the num. of required resources is reached.
      for (unsigned sym_idx = 0; sym_idx + nof_f2_symbols <= max_nof_symbols.value(); sym_idx += nof_f2_symbols) {
        const ofdm_symbol_range symbols{sym_idx, sym_idx + nof_f2_symbols};

        // Allocate resources for first hop.
        res_list.emplace_back(pucch_grant{
            .format = pucch_format::FORMAT_2, .symbols = symbols, .prbs = prbs, .freq_hop_grant = freq_hop_prbs});
        if (res_list.size() == nof_res_f2) {
          break;
        }

        // Allocate resources for second hop.
        res_list.emplace_back(pucch_grant{
            .format = pucch_format::FORMAT_2, .symbols = symbols, .prbs = freq_hop_prbs, .freq_hop_grant = prbs});
        if (res_list.size() == nof_res_f2) {
          break;
        }
      }
      if (res_list.size() == nof_res_f2) {
        break;
      }
    }
  }
  // Without intraslot freq. hopping.
  else {
    for (unsigned rb_idx = 0, rb_stop = bwp_size_rbs / 2 - f2_max_rbs; rb_idx < rb_stop; rb_idx += f2_max_rbs) {
      const prb_interval prbs_low_spectrum{rb_idx, rb_idx + f2_max_rbs};
      // Pre-compute the RBs for the resources to be allocated on the upper part of the spectrum. This is to achieve
      // balancing of the PUCCH resources on both sides of the BWP.
      const prb_interval prbs_hi_spectrum{bwp_size_rbs - f2_max_rbs - rb_idx, bwp_size_rbs - rb_idx};

      // Generate resource for increasing Symbol index, until the num. of required resources is reached.
      for (unsigned sym_idx = 0; sym_idx + nof_f2_symbols <= max_nof_symbols.value(); sym_idx += nof_f2_symbols) {
        const ofdm_symbol_range symbols{sym_idx, sym_idx + nof_f2_symbols};
        res_list.emplace_back(
            pucch_grant{.format = pucch_format::FORMAT_2, .symbols = symbols, .prbs = prbs_low_spectrum});
        if (res_list.size() == nof_res_f2) {
          break;
        }
      }
      if (res_list.size() == nof_res_f2) {
        break;
      }

      // Repeat the resource allocation on the upper part of the spectrum, to spread the PUCCH resource on both sides of
      // the BWP.
      for (unsigned sym_idx = 0; sym_idx + nof_f2_symbols <= NOF_OFDM_SYM_PER_SLOT_NORMAL_CP;
           sym_idx += nof_f2_symbols) {
        const ofdm_symbol_range symbols{sym_idx, sym_idx + nof_f2_symbols};
        res_list.emplace_back(
            pucch_grant{.format = pucch_format::FORMAT_2, .symbols = symbols, .prbs = prbs_hi_spectrum});
        if (res_list.size() == nof_res_f2) {
          break;
        }
      }

      if (res_list.size() == nof_res_f2) {
        break;
      }
    }
  }

  return res_list;
}

static std::vector<pucch_grant> compute_f3_res(unsigned                         nof_res_f3,
                                               const pucch_f3_params&           params,
                                               unsigned                         bwp_size_rbs,
                                               bounded_integer<unsigned, 1, 14> max_nof_symbols)
{
  // Compute the number of symbols and RBs for F3.
  std::vector<pucch_grant> res_list;
  const unsigned           nof_f3_symbols = params.nof_syms.value();
  const unsigned           f3_max_rbs     = params.max_payload_bits.has_value()
                                                ? get_pucch_format3_max_nof_prbs(params.max_payload_bits.value(),
                                                                   nof_f3_symbols,
                                                                   to_max_code_rate_float(params.max_code_rate),
                                                                   params.intraslot_freq_hopping,
                                                                   params.additional_dmrs,
                                                                   params.pi2_bpsk)
                                                : params.max_nof_rbs.value();

  if (f3_max_rbs > pucch_constants::f3::MAX_NOF_RBS) {
    return {};
  }

  // With intraslot freq. hopping.
  if (params.intraslot_freq_hopping) {
    for (unsigned rb_idx = 0, rb_stop = bwp_size_rbs / 2 - f3_max_rbs; rb_idx < rb_stop; rb_idx += f3_max_rbs) {
      // Generate resource for increasing RB index, until the num. of required resources is reached.
      const prb_interval prbs{rb_idx, rb_idx + f3_max_rbs};
      // Pre-compute the second RBs for the frequency hop specularly to the first RBs.
      const prb_interval freq_hop_prbs{bwp_size_rbs - f3_max_rbs - rb_idx, bwp_size_rbs - rb_idx};

      // Generate resource for increasing Symbol index, until the num. of required resources is reached.
      for (unsigned sym_idx = 0; sym_idx + nof_f3_symbols <= max_nof_symbols.value(); sym_idx += nof_f3_symbols) {
        const ofdm_symbol_range symbols{sym_idx, sym_idx + nof_f3_symbols};

        // Allocate resources for first hop.
        res_list.emplace_back(pucch_grant{
            .format = pucch_format::FORMAT_3, .symbols = symbols, .prbs = prbs, .freq_hop_grant = freq_hop_prbs});
        if (res_list.size() == nof_res_f3) {
          break;
        }

        // Allocate resources for second hop.
        res_list.emplace_back(pucch_grant{
            .format = pucch_format::FORMAT_3, .symbols = symbols, .prbs = freq_hop_prbs, .freq_hop_grant = prbs});
        if (res_list.size() == nof_res_f3) {
          break;
        }
      }
      if (res_list.size() == nof_res_f3) {
        break;
      }
    }
  }
  // Without intraslot freq. hopping.
  else {
    for (unsigned rb_idx = 0, rb_stop = bwp_size_rbs / 2 - f3_max_rbs; rb_idx < rb_stop; rb_idx += f3_max_rbs) {
      const prb_interval prbs_low_spectrum{rb_idx, rb_idx + f3_max_rbs};
      // Pre-compute the RBs for the resources to be allocated on the upper part of the spectrum. This is to achieve
      // balancing of the PUCCH resources on both sides of the BWP.
      const prb_interval prbs_hi_spectrum{bwp_size_rbs - f3_max_rbs - rb_idx, bwp_size_rbs - rb_idx};

      // Generate resource for increasing Symbol index, until the num. of required resources is reached.
      for (unsigned sym_idx = 0; sym_idx + nof_f3_symbols <= max_nof_symbols.value(); sym_idx += nof_f3_symbols) {
        const ofdm_symbol_range symbols{sym_idx, sym_idx + nof_f3_symbols};
        res_list.emplace_back(
            pucch_grant{.format = pucch_format::FORMAT_3, .symbols = symbols, .prbs = prbs_low_spectrum});
        if (res_list.size() == nof_res_f3) {
          break;
        }
      }
      if (res_list.size() == nof_res_f3) {
        break;
      }

      // Repeat the resource allocation on the upper part of the spectrum, to spread the PUCCH resource on both sides of
      // the BWP.
      for (unsigned sym_idx = 0; sym_idx + nof_f3_symbols <= NOF_OFDM_SYM_PER_SLOT_NORMAL_CP;
           sym_idx += nof_f3_symbols) {
        const ofdm_symbol_range symbols{sym_idx, sym_idx + nof_f3_symbols};
        res_list.emplace_back(
            pucch_grant{.format = pucch_format::FORMAT_3, .symbols = symbols, .prbs = prbs_hi_spectrum});
        if (res_list.size() == nof_res_f3) {
          break;
        }
      }

      if (res_list.size() == nof_res_f3) {
        break;
      }
    }
  }

  return res_list;
}

static std::vector<pucch_grant> compute_f4_res(unsigned                         nof_res_f4,
                                               const pucch_f4_params&           params,
                                               unsigned                         bwp_size_rbs,
                                               bounded_integer<unsigned, 1, 14> max_nof_symbols)
{
  std::vector<pucch_grant>  res_list;
  const unsigned            nof_f4_symbols = params.nof_syms.value();
  const unsigned            nof_occs       = params.occ_supported ? static_cast<unsigned>(params.occ_length) : 1U;
  static constexpr unsigned f4_max_rbs     = 1U;

  // With intraslot freq. hopping.
  if (params.intraslot_freq_hopping) {
    for (unsigned rb_idx = 0, rb_stop = bwp_size_rbs / 2 - f4_max_rbs; rb_idx != rb_stop; ++rb_idx) {
      // Generate resource for increasing RB index, until the num. of required resources is reached.
      const prb_interval prbs{rb_idx, rb_idx + f4_max_rbs};
      // Pre-compute the second RBs for the frequency hop specularly to the first RBs.
      const prb_interval freq_hop_prbs{bwp_size_rbs - f4_max_rbs - rb_idx, bwp_size_rbs - rb_idx};

      // Generate resource for increasing Symbol index, until the num. of required resources is reached.
      for (unsigned sym_idx = 0; sym_idx + nof_f4_symbols <= max_nof_symbols.value(); sym_idx += nof_f4_symbols) {
        const ofdm_symbol_range symbols{sym_idx, sym_idx + nof_f4_symbols};

        for (unsigned occ_index = 0; occ_index != nof_occs; ++occ_index) {
          // Allocate resources for first hop.
          res_list.emplace_back(pucch_grant{.format         = pucch_format::FORMAT_4,
                                            .symbols        = symbols,
                                            .prbs           = prbs,
                                            .freq_hop_grant = freq_hop_prbs,
                                            .occ_cs_idx     = occ_index});
          if (res_list.size() == nof_res_f4) {
            return res_list;
          }

          // Allocate resources for second hop.
          res_list.emplace_back(pucch_grant{.format         = pucch_format::FORMAT_4,
                                            .symbols        = symbols,
                                            .prbs           = freq_hop_prbs,
                                            .freq_hop_grant = prbs,
                                            .occ_cs_idx     = occ_index});
          if (res_list.size() == nof_res_f4) {
            return res_list;
          }
        }
      }
    }
  }
  // Without intraslot freq. hopping.
  else {
    for (unsigned rb_idx = 0, rb_stop = bwp_size_rbs / 2 - f4_max_rbs; rb_idx != rb_stop; ++rb_idx) {
      const prb_interval prbs_low_spectrum{rb_idx, rb_idx + f4_max_rbs};
      // Pre-compute the RBs for the resources to be allocated on the upper part of the spectrum. This is to achieve
      // balancing of the PUCCH resources on both sides of the BWP.
      const prb_interval prbs_hi_spectrum{bwp_size_rbs - f4_max_rbs - rb_idx, bwp_size_rbs - rb_idx};

      // Generate resource for increasing Symbol index, until the num. of required resources is reached.
      for (unsigned sym_idx = 0; sym_idx + nof_f4_symbols <= max_nof_symbols.value(); sym_idx += nof_f4_symbols) {
        const ofdm_symbol_range symbols{sym_idx, sym_idx + nof_f4_symbols};
        for (unsigned occ_index = 0; occ_index != nof_occs; ++occ_index) {
          res_list.emplace_back(pucch_grant{.format     = pucch_format::FORMAT_4,
                                            .symbols    = symbols,
                                            .prbs       = prbs_low_spectrum,
                                            .occ_cs_idx = occ_index});
          if (res_list.size() == nof_res_f4) {
            return res_list;
          }
        }
      }

      // Repeat the resource allocation on the upper part of the spectrum, to spread the PUCCH resource on both sides of
      // the BWP.
      for (unsigned sym_idx = 0; sym_idx + nof_f4_symbols <= max_nof_symbols.value(); sym_idx += nof_f4_symbols) {
        const ofdm_symbol_range symbols{sym_idx, sym_idx + nof_f4_symbols};
        for (unsigned occ_index = 0; occ_index != nof_occs; ++occ_index) {
          res_list.emplace_back(pucch_grant{
              .format = pucch_format::FORMAT_4, .symbols = symbols, .prbs = prbs_hi_spectrum, .occ_cs_idx = occ_index});
          if (res_list.size() == nof_res_f4) {
            return res_list;
          }
        }
      }
    }
  }

  return res_list;
}

error_type<std::string> config_helpers::pucch_parameters_validator(const pucch_resource_builder_params& params,
                                                                   unsigned                             bwp_size_rbs)
{
  if (params.nof_cell_sr_resources == 0) {
    return make_unexpected("The number of PUCCH SR resources must be greater than zero.");
  }
  if (params.nof_cell_res_set_configs == 0) {
    return make_unexpected("The number of PUCCH resource set configurations must be greater than zero.");
  }

  const unsigned max_nof_symbols = params.max_nof_symbols.value();

  if (params.format_01() == pucch_format::FORMAT_0 and params.format_234() == pucch_format::FORMAT_2 and
      (params.res_set_0_size > 6 or params.res_set_1_size > 6)) {
    return make_unexpected("When using PUCCH Formats 0 and 2, resource set size cannot be greater than 6, as 2 "
                           "resources in each set are reserved.");
  }

  const unsigned nof_res_f0_f1 =
      params.nof_cell_sr_resources + params.nof_cell_res_set_configs * params.res_set_0_size.value();
  unsigned nof_rbs_f0_f1;
  if (std::holds_alternative<pucch_f0_params>(params.f0_or_f1_params)) {
    const auto& f0_params = std::get<pucch_f0_params>(params.f0_or_f1_params);
    if (f0_params.intraslot_freq_hopping and f0_params.nof_syms == 1) {
      return make_unexpected("Intra-slot frequency hopping for PUCCH Format 0 requires 2 symbols");
    }

    if (params.format_234() != pucch_format::FORMAT_2) {
      return make_unexpected("PUCCH Formats 3/4 not currently supported with PUCCH Format 0");
    }

    // We define a block as a set of resources of the same format aligned over the same starting PRB.
    const unsigned nof_f0_blocks = max_nof_symbols / f0_params.nof_syms.value();
    nof_rbs_f0_f1                = divide_ceil(nof_res_f0_f1, nof_f0_blocks);
    // With intraslot_freq_hopping, the number of RBs is even. Round up to the nearest even number if it's odd.
    if (f0_params.intraslot_freq_hopping and (nof_rbs_f0_f1 & 1U) != 0) {
      nof_rbs_f0_f1 += 1;
    }
  } else {
    const auto& f1_params = std::get<pucch_f1_params>(params.f0_or_f1_params);
    if (f1_params.nof_syms > max_nof_symbols) {
      return make_unexpected("The number of symbols for PUCCH Format 1 exceeds the maximum number of symbols available "
                             "for PUCCH resources");
    }

    const unsigned nof_occ_codes = f1_params.occ_supported ? format1_symb_to_spreading_factor(f1_params.nof_syms) : 1;
    const unsigned nof_css       = format1_cp_step_to_uint(f1_params.nof_cyc_shifts);
    // We define a block as a set of resources of the same format aligned over the same starting PRB.
    const unsigned nof_f1_blocks = nof_occ_codes * nof_css * (max_nof_symbols / f1_params.nof_syms.value());
    nof_rbs_f0_f1                = divide_ceil(nof_res_f0_f1, nof_f1_blocks);
    // With intraslot_freq_hopping, the number of RBs is even. Round up to the nearest even number if it's odd.
    if (f1_params.intraslot_freq_hopping and (nof_rbs_f0_f1 & 1U) != 0) {
      nof_rbs_f0_f1 += 1;
    }
  }

  const unsigned nof_res_f2_f3_f4 =
      params.nof_cell_csi_resources + params.nof_cell_res_set_configs * params.res_set_1_size.value();
  unsigned nof_rbs_f2_f3_f4 = 0;
  if (std::holds_alternative<pucch_f2_params>(params.f2_or_f3_or_f4_params)) {
    const auto& f2_params = std::get<pucch_f2_params>(params.f2_or_f3_or_f4_params);

    if (f2_params.intraslot_freq_hopping and f2_params.nof_syms == 1) {
      return make_unexpected("Intra-slot frequency hopping for PUCCH Format 2 requires 2 symbols");
    }

    const unsigned f2_max_rbs = f2_params.max_payload_bits.has_value()
                                    ? get_pucch_format2_max_nof_prbs(f2_params.max_payload_bits.value(),
                                                                     f2_params.nof_syms.value(),
                                                                     to_max_code_rate_float(f2_params.max_code_rate))
                                    : f2_params.max_nof_rbs.value();
    if (f2_max_rbs > pucch_constants::f2::MAX_NOF_RBS) {
      return make_unexpected("The configured maximum number of RBs for PUCCH Format 2 exceeds the limit of 16");
    }

    // We define a block as a set of resources of the same format aligned over the same starting PRB.
    const unsigned nof_f2_blocks = params.max_nof_symbols.value() / f2_params.nof_syms.value();
    nof_rbs_f2_f3_f4             = divide_ceil(nof_res_f2_f3_f4, nof_f2_blocks) * f2_max_rbs;
    // With intraslot_freq_hopping, the number of RBs is even. Round up to the nearest even number if it's odd.
    if (f2_params.intraslot_freq_hopping and (nof_rbs_f2_f3_f4 & 1U) != 0) {
      nof_rbs_f2_f3_f4 += 1;
    }
  } else if (std::holds_alternative<pucch_f3_params>(params.f2_or_f3_or_f4_params)) {
    const auto& f3_params = std::get<pucch_f3_params>(params.f2_or_f3_or_f4_params);

    if (f3_params.nof_syms.value() > max_nof_symbols) {
      return make_unexpected("The number of symbols for PUCCH Format 3 exceeds the maximum number of symbols available "
                             "for PUCCH resources");
    }

    const unsigned f3_max_rbs = f3_params.max_payload_bits.has_value()
                                    ? get_pucch_format3_max_nof_prbs(f3_params.max_payload_bits.value(),
                                                                     f3_params.nof_syms.value(),
                                                                     to_max_code_rate_float(f3_params.max_code_rate),
                                                                     f3_params.intraslot_freq_hopping,
                                                                     f3_params.additional_dmrs,
                                                                     f3_params.pi2_bpsk)
                                    : f3_params.max_nof_rbs.value();
    if (f3_max_rbs > pucch_constants::f3::MAX_NOF_RBS) {
      return make_unexpected("The number of PRBs for PUCCH Format 3 exceeds the limit of 16");
    }

    // We define a block as a set of resources of the same format aligned over the same starting PRB.
    const unsigned nof_f3_blocks = max_nof_symbols / f3_params.nof_syms.value();
    nof_rbs_f2_f3_f4             = divide_ceil(nof_res_f2_f3_f4, nof_f3_blocks) * f3_max_rbs;
    // With intraslot_freq_hopping, the number of RBs is even. Round up to the nearest even number if it's odd.
    if (f3_params.intraslot_freq_hopping and (nof_rbs_f2_f3_f4 & 1U) != 0) {
      nof_rbs_f2_f3_f4 += 1;
    }
  } else {
    const auto& f4_params = std::get<pucch_f4_params>(params.f2_or_f3_or_f4_params);

    const unsigned nof_occs = f4_params.occ_supported ? static_cast<unsigned>(f4_params.occ_length) : 1U;
    // We define a block as a set of resources of the same format aligned over the same starting PRB.
    const unsigned nof_f4_blocks = nof_occs * max_nof_symbols / f4_params.nof_syms.value();
    nof_rbs_f2_f3_f4             = divide_ceil(nof_res_f2_f3_f4, nof_f4_blocks);
    // With intraslot_freq_hopping, the number of RBs is even. Round up to the nearest even number if it's odd.
    if (f4_params.intraslot_freq_hopping and (nof_rbs_f2_f3_f4 & 1U) != 0) {
      nof_rbs_f2_f3_f4 += 1;
    }
  }

  // Verify the number of RBs for the PUCCH resources does not exceed the BWP size.
  // [Implementation-defined] We do not allow the PUCCH resources to occupy more than 50% of the BWP. This is an
  // extreme case, and ideally the PUCCH configuration should result in a much lower PRBs usage.
  static constexpr float max_allowed_rb_usage = 0.5F;
  if (nof_rbs_f0_f1 + nof_rbs_f2_f3_f4 >= max_allowed_rb_usage * bwp_size_rbs) {
    return make_unexpected("With the given parameters, the number of PRBs for PUCCH exceeds the 50% of the BWP PRBs");
  }
  return {};
}

static std::vector<pucch_resource>
merge_f0_f1_f2_f3_f4_resource_lists(const std::vector<pucch_grant>&      pucch_f0_f1_resource_list,
                                    const std::vector<pucch_grant>&      pucch_f2_f3_f4_resource_list,
                                    const pucch_resource_builder_params& params,
                                    unsigned                             bwp_size_rbs)
{
  // This function merges the lists of PUCCH F0/F1 and F2/F3/F4 resource. It first allocates the F0/F1 resources on
  // the sides of the BWP; second, it allocates the F2/F3/F4 resources beside F0/F1 ones.
  std::vector<pucch_resource> resource_list;

  // NOTE: PUCCH F0/F1 resource are located at the sides of the BWP. PUCCH F2/F3/F4 are located beside the F0/F1
  // resources, specifically on F0/F1's right (on the frequency axis) for frequencies < BWP/2, and F0/F1's left (on
  // the frequency axis) for frequencies > BWP/2 and < BWP.
  unsigned f0_f1_rbs_occupancy_low_freq = 0;
  unsigned f0_f1_rbs_occupancy_hi_freq  = 0;

  if (std::holds_alternative<pucch_f0_params>(params.f0_or_f1_params)) {
    for (const auto& res_f0 : pucch_f0_f1_resource_list) {
      auto res_id = static_cast<unsigned>(resource_list.size());
      // No need to set res_id.second, which is the PUCCH resource ID for the ASN1 message. This will be set by the DU
      // before assigning the resources to the UE.
      pucch_resource res{.res_id = {res_id, 0}, .starting_prb = res_f0.prbs.start()};
      if (res_f0.freq_hop_grant.has_value()) {
        res.second_hop_prb.emplace(res_f0.freq_hop_grant.value().start());
      }
      pucch_format_0_cfg format0{.initial_cyclic_shift = 0U};

      // Update the frequency shift for PUCCH F2/F3/F4.
      if (res_f0.prbs.start() < bwp_size_rbs / 2 - 1U) {
        // f0_f1_rbs_occupancy_low_freq accounts for the PUCCH F0/F1 resource occupancy on the first half of the BWP;
        // PUCCH F0/F1 resources are located on the lowest RBs indices.
        f0_f1_rbs_occupancy_low_freq = std::max(f0_f1_rbs_occupancy_low_freq, res_f0.prbs.start() + 1);
        if (res_f0.freq_hop_grant.has_value()) {
          f0_f1_rbs_occupancy_hi_freq =
              std::max(f0_f1_rbs_occupancy_hi_freq, bwp_size_rbs - res_f0.freq_hop_grant.value().start());
        }
      } else if (res_f0.prbs.start() > bwp_size_rbs / 2) {
        // f0_f1_rbs_occupancy_hi_freq accounts for the PUCCH F0/F1 resource occupancy on the second half of the BWP;
        // PUCCH F0/F1 resources are located on the highest RBs indices.
        f0_f1_rbs_occupancy_hi_freq = std::max(f0_f1_rbs_occupancy_hi_freq, bwp_size_rbs - res_f0.prbs.start());
        if (res_f0.freq_hop_grant.has_value()) {
          f0_f1_rbs_occupancy_low_freq = std::max(f0_f1_rbs_occupancy_low_freq, res_f0.freq_hop_grant.value().start());
        }
      } else {
        ocudu_assert(false, "PUCCH resources are not expected to be allocated at the centre of the BWP");
        return {};
      }

      res.nof_symbols      = res_f0.symbols.length();
      res.starting_sym_idx = res_f0.symbols.start();
      res.format_params.emplace<pucch_format_0_cfg>(format0);
      res.format = pucch_format::FORMAT_0;
      resource_list.emplace_back(res);
    }
  } else {
    for (const auto& res_f1 : pucch_f0_f1_resource_list) {
      auto res_id = static_cast<unsigned>(resource_list.size());
      // No need to set res_id.second, which is the PUCCH resource ID for the ASN1 message. This will be set by the DU
      // before assigning the resources to the UE.
      pucch_resource res{.res_id = {res_id, 0}, .starting_prb = res_f1.prbs.start()};
      if (res_f1.freq_hop_grant.has_value()) {
        res.second_hop_prb.emplace(res_f1.freq_hop_grant.value().start());
      }
      pucch_format_1_cfg format1;

      // Update the frequency shift for PUCCH F2/F3/F4.
      if (res_f1.prbs.start() < bwp_size_rbs / 2 - 1) {
        // f0_f1_rbs_occupancy_low_freq accounts for the PUCCH F0/F1 resource occupancy on the first half of the BWP;
        // PUCCH F0/F1 resources are located on the lowest RBs indices.
        f0_f1_rbs_occupancy_low_freq = std::max(f0_f1_rbs_occupancy_low_freq, res_f1.prbs.start() + 1);
        if (res_f1.freq_hop_grant.has_value()) {
          f0_f1_rbs_occupancy_hi_freq =
              std::max(f0_f1_rbs_occupancy_hi_freq, bwp_size_rbs - res_f1.freq_hop_grant.value().start());
        }
      } else if (res_f1.prbs.start() > bwp_size_rbs / 2) {
        // f0_f1_rbs_occupancy_hi_freq accounts for the PUCCH F0/F1 resource occupancy on the second half of the BWP;
        // PUCCH F0/F1 resources are located on the highest RBs indices.
        f0_f1_rbs_occupancy_hi_freq = std::max(f0_f1_rbs_occupancy_hi_freq, bwp_size_rbs - res_f1.prbs.start());
        if (res_f1.freq_hop_grant.has_value()) {
          f0_f1_rbs_occupancy_low_freq = std::max(f0_f1_rbs_occupancy_low_freq, res_f1.freq_hop_grant.value().start());
        }
      } else {
        ocudu_assert(false, "PUCCH resources are not expected to be allocated at the centre of the BWP");
        return {};
      }

      res.nof_symbols      = res_f1.symbols.length();
      res.starting_sym_idx = res_f1.symbols.start();
      ocudu_assert(res_f1.occ_cs_idx.has_value(),
                   "The index needed to compute OCC code and cyclic shift have not been found");
      const auto&    f1_params     = std::get<pucch_f1_params>(params.f0_or_f1_params);
      const unsigned nof_css       = format1_cp_step_to_uint(f1_params.nof_cyc_shifts);
      format1.initial_cyclic_shift = occ_cs_index_to_cyclic_shift(res_f1.occ_cs_idx.value(), nof_css);
      format1.time_domain_occ      = occ_cs_index_to_occ(res_f1.occ_cs_idx.value(), nof_css);
      res.format_params.emplace<pucch_format_1_cfg>(format1);
      res.format = pucch_format::FORMAT_1;
      resource_list.emplace_back(res);
    }
  }

  if (std::holds_alternative<pucch_f2_params>(params.f2_or_f3_or_f4_params)) {
    for (const auto& res_f2 : pucch_f2_f3_f4_resource_list) {
      auto res_id = static_cast<unsigned>(resource_list.size());
      // No need to set res_id.second, which is the PUCCH resource ID for the ASN1 message. This will be set by the
      // DU before assigning the resources to the UE.
      pucch_resource res{.res_id = {res_id, 0}};
      // Shift F2 RBs depending on previously allocated F0/F1 resources.
      if (res_f2.prbs.start() < bwp_size_rbs / 2 - res_f2.prbs.length()) {
        res.starting_prb = res_f2.prbs.start() + f0_f1_rbs_occupancy_low_freq;
        if (res_f2.freq_hop_grant.has_value()) {
          res.second_hop_prb.emplace(res_f2.freq_hop_grant.value().start() - f0_f1_rbs_occupancy_hi_freq);
        }
      } else if (res_f2.prbs.start() > bwp_size_rbs / 2) {
        res.starting_prb = res_f2.prbs.start() - f0_f1_rbs_occupancy_hi_freq;
        if (res_f2.freq_hop_grant.has_value()) {
          res.second_hop_prb.emplace(res_f2.freq_hop_grant.value().start() + f0_f1_rbs_occupancy_low_freq);
        }
      } else {
        ocudu_assert(false, "PUCCH resources are not expected to be allocated at the centre of the BWP");
        return {};
      }

      res.nof_symbols      = res_f2.symbols.length();
      res.starting_sym_idx = res_f2.symbols.start();
      pucch_format_2_3_cfg format2;
      format2.nof_prbs = res_f2.prbs.length();
      res.format_params.emplace<pucch_format_2_3_cfg>(format2);
      res.format = pucch_format::FORMAT_2;
      resource_list.emplace_back(res);
    }
  } else if (std::holds_alternative<pucch_f3_params>(params.f2_or_f3_or_f4_params)) {
    for (const auto& res_f3 : pucch_f2_f3_f4_resource_list) {
      auto res_id = static_cast<unsigned>(resource_list.size());
      // No need to set res_id.second, which is the PUCCH resource ID for the ASN1 message. This will be set by the
      // DU before assigning the resources to the UE.
      pucch_resource res{.res_id = {res_id, 0}};
      // Shift F3 RBs depending on previously allocated F0/F1 resources.
      if (res_f3.prbs.start() < bwp_size_rbs / 2 - res_f3.prbs.length()) {
        res.starting_prb = res_f3.prbs.start() + f0_f1_rbs_occupancy_low_freq;
        if (res_f3.freq_hop_grant.has_value()) {
          res.second_hop_prb.emplace(res_f3.freq_hop_grant.value().start() - f0_f1_rbs_occupancy_hi_freq);
        }
      } else if (res_f3.prbs.start() > bwp_size_rbs / 2) {
        res.starting_prb = res_f3.prbs.start() - f0_f1_rbs_occupancy_hi_freq;
        if (res_f3.freq_hop_grant.has_value()) {
          res.second_hop_prb.emplace(res_f3.freq_hop_grant.value().start() + f0_f1_rbs_occupancy_low_freq);
        }
      } else {
        ocudu_assert(false, "PUCCH resources are not expected to be allocated at the centre of the BWP");
        return {};
      }

      res.nof_symbols      = res_f3.symbols.length();
      res.starting_sym_idx = res_f3.symbols.start();
      pucch_format_2_3_cfg format3;
      format3.nof_prbs = res_f3.prbs.length();
      res.format_params.emplace<pucch_format_2_3_cfg>(format3);
      res.format = pucch_format::FORMAT_3;
      resource_list.emplace_back(res);
    }
  } else {
    for (const auto& res_f4 : pucch_f2_f3_f4_resource_list) {
      auto res_id = static_cast<unsigned>(resource_list.size());
      // No need to set res_id.second, which is the PUCCH resource ID for the ASN1 message. This will be set by the
      // DU before assigning the resources to the UE.
      pucch_resource res{.res_id = {res_id, 0}};
      // Shift F4 RBs depending on previously allocated F0/F1 resources.
      if (res_f4.prbs.start() < bwp_size_rbs / 2 - res_f4.prbs.length()) {
        res.starting_prb = res_f4.prbs.start() + f0_f1_rbs_occupancy_low_freq;
        if (res_f4.freq_hop_grant.has_value()) {
          res.second_hop_prb.emplace(res_f4.freq_hop_grant.value().start() - f0_f1_rbs_occupancy_hi_freq);
        }
      } else if (res_f4.prbs.start() > bwp_size_rbs / 2) {
        res.starting_prb = res_f4.prbs.start() - f0_f1_rbs_occupancy_hi_freq;
        if (res_f4.freq_hop_grant.has_value()) {
          res.second_hop_prb.emplace(res_f4.freq_hop_grant.value().start() + f0_f1_rbs_occupancy_low_freq);
        }
      } else {
        ocudu_assert(false, "PUCCH resources are not expected to be allocated at the centre of the BWP");
        return {};
      }

      res.nof_symbols      = res_f4.symbols.length();
      res.starting_sym_idx = res_f4.symbols.start();

      const auto&        f4_params = std::get<pucch_f4_params>(params.f2_or_f3_or_f4_params);
      pucch_format_4_cfg format4;
      format4.occ_length = f4_params.occ_length;
      format4.occ_index  = occ_cs_index_to_f4_occ(res_f4.occ_cs_idx.value());
      res.format_params.emplace<pucch_format_4_cfg>(format4);
      res.format = pucch_format::FORMAT_4;
      resource_list.emplace_back(res);
    }
  }

  return resource_list;
}

static bool validate_generated_list(const std::vector<pucch_resource>&   res_list,
                                    const pucch_resource_builder_params& params)
{
  auto           expected_res_by_format = std::array<unsigned, 5>{0, 0, 0, 0, 0};
  const unsigned expected_res_01 =
      params.nof_cell_sr_resources + params.nof_cell_res_set_configs * params.res_set_0_size.value();
  if (params.format_01() == pucch_format::FORMAT_0) {
    expected_res_by_format[0] = expected_res_01;
  } else {
    expected_res_by_format[1] = expected_res_01;
  }
  const unsigned expected_res_234 =
      params.nof_cell_csi_resources + params.nof_cell_res_set_configs * params.res_set_1_size.value();
  switch (params.format_234()) {
    case pucch_format::FORMAT_2:
      expected_res_by_format[2] = expected_res_234;
      break;
    case pucch_format::FORMAT_3:
      expected_res_by_format[3] = expected_res_234;
      break;
    case pucch_format::FORMAT_4:
      expected_res_by_format[4] = expected_res_234;
      break;
    default:
      ocudu_assertion_failure("Unexpected PUCCH format for F2/F3/F4 resources");
      break;
  }

  std::array<unsigned, 5> res_count_by_format{0, 0, 0, 0, 0};
  for (const auto& res : res_list) {
    switch (res.format) {
      case pucch_format::FORMAT_0:
        ++res_count_by_format[0];
        break;
      case pucch_format::FORMAT_1:
        ++res_count_by_format[1];
        break;
      case pucch_format::FORMAT_2:
        ++res_count_by_format[2];
        break;
      case pucch_format::FORMAT_3:
        ++res_count_by_format[3];
        break;
      case pucch_format::FORMAT_4:
        ++res_count_by_format[4];
        break;
      default:
        break;
    }
  }
  if (expected_res_by_format != res_count_by_format) {
    ocudu_assertion_failure("The number of PUCCH resources by format in the generated list does not match the expected "
                            "one. Expected: {{{}}}, actual: {{{}}}",
                            fmt::join(expected_res_by_format, ", "),
                            fmt::join(res_count_by_format, ", "));
    return false;
  }

  for (unsigned res_idx = 0; res_idx != expected_res_01; ++res_idx) {
    if (res_list[res_idx].format == pucch_format::FORMAT_2 or res_list[res_idx].format == pucch_format::FORMAT_3 or
        res_list[res_idx].format == pucch_format::FORMAT_4) {
      ocudu_assertion_failure(
          "The F0/F1 resources in the cell PUCCH resource list must precede all F2/F3/F4 resources.");
      return false;
    }
  }
  return true;
}

std::vector<pucch_resource> config_helpers::generate_cell_pucch_res_list(const pucch_resource_builder_params& params,
                                                                         unsigned bwp_size_rbs)
{
  const unsigned nof_res_f0_f1 =
      params.nof_cell_sr_resources + params.nof_cell_res_set_configs * params.res_set_0_size.value();
  const unsigned nof_res_f2_f3_f4 =
      params.nof_cell_csi_resources + params.nof_cell_res_set_configs * params.res_set_1_size.value();
  const unsigned max_nof_symbols = params.max_nof_symbols.value();
  auto           outcome         = pucch_parameters_validator(params, bwp_size_rbs);
  if (not outcome.has_value()) {
    ocudu_assertion_failure("The cell list could not be generated due to: {}", outcome.error());
    return {};
  }

  // Compute the PUCCH F0/F1 and F2/F3/F4 resources separately.
  std::vector<pucch_grant> f0_f1_res_list;
  if (std::holds_alternative<pucch_f0_params>(params.f0_or_f1_params)) {
    const auto& f0_params = std::get<pucch_f0_params>(params.f0_or_f1_params);
    f0_f1_res_list        = compute_f0_res(nof_res_f0_f1, f0_params, bwp_size_rbs, max_nof_symbols);
  } else {
    const auto&    f1_params     = std::get<pucch_f1_params>(params.f0_or_f1_params);
    const unsigned nof_occ_codes = f1_params.occ_supported ? format1_symb_to_spreading_factor(f1_params.nof_syms) : 1;
    const unsigned nof_css       = format1_cp_step_to_uint(f1_params.nof_cyc_shifts);
    f0_f1_res_list = compute_f1_res(nof_res_f0_f1, f1_params, bwp_size_rbs, nof_css * nof_occ_codes, max_nof_symbols);
  }

  std::vector<pucch_grant> f2_f3_f4_res_list;
  if (std::holds_alternative<pucch_f2_params>(params.f2_or_f3_or_f4_params)) {
    const auto& f2_params = std::get<pucch_f2_params>(params.f2_or_f3_or_f4_params);
    f2_f3_f4_res_list     = compute_f2_res(nof_res_f2_f3_f4, f2_params, bwp_size_rbs, max_nof_symbols);
  } else if (std::holds_alternative<pucch_f3_params>(params.f2_or_f3_or_f4_params)) {
    const auto& f3_params = std::get<pucch_f3_params>(params.f2_or_f3_or_f4_params);
    f2_f3_f4_res_list     = compute_f3_res(nof_res_f2_f3_f4, f3_params, bwp_size_rbs, max_nof_symbols);
  } else {
    const auto& f4_params = std::get<pucch_f4_params>(params.f2_or_f3_or_f4_params);
    f2_f3_f4_res_list     = compute_f4_res(nof_res_f2_f3_f4, f4_params, bwp_size_rbs, max_nof_symbols);
  }

  auto res_list = merge_f0_f1_f2_f3_f4_resource_lists(f0_f1_res_list, f2_f3_f4_res_list, params, bwp_size_rbs);

  if (res_list.size() > pucch_constants::MAX_NOF_CELL_DED_RESOURCES) {
    ocudu_assertion_failure("With the given parameters, the number of PUCCH resources generated for the "
                            "cell exceeds maximum supported limit of {}",
                            pucch_constants::MAX_NOF_CELL_DED_RESOURCES);
    return {};
  }

  if (not validate_generated_list(res_list, params)) {
    return {};
  }

  return res_list;
}
