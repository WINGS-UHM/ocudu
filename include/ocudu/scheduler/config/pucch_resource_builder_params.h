// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#pragma once

#include "ocudu/adt/bounded_integer.h"
#include "ocudu/ran/frame_types.h"
#include "ocudu/ran/pucch/pucch_configuration.h"
#include "ocudu/ran/pucch/pucch_constants.h"
#include <optional>
#include <variant>

namespace ocudu {

/// Parameters for the generation of PUCCH Format 0 resources.
struct pucch_f0_params {
  using nof_symbols =
      bounded_integer<unsigned, pucch_constants::f0::NOF_SYMS.start(), pucch_constants::f0::NOF_SYMS.stop()>;

  /// Number of OFDM symbols.
  nof_symbols nof_syms{pucch_constants::f0::NOF_SYMS.stop()};
  /// Whether to configure intraslot frequency hopping.
  bool intraslot_freq_hopping{false};
};

/// \brief Options for the number of Initial Cyclic Shifts that can be set for PUCCH Format 1.
///
/// Defines the number of different Initial Cyclic Shifts that can be used for PUCCH Format 1, as per \c PUCCH-format1,
/// in \c PUCCH-Config, TS 38.331. We assume the CS are evenly distributed, which means we can only have a divisor of 12
/// possible cyclic shifts.
enum class pucch_nof_cyclic_shifts { no_cyclic_shift = 1, two = 2, three = 3, four = 4, six = 6, twelve = 12 };

inline unsigned format1_cp_step_to_uint(pucch_nof_cyclic_shifts opt)
{
  return static_cast<unsigned>(opt);
}

/// Parameters for the generation of PUCCH Format 1 resources.
struct pucch_f1_params {
  using nof_symbols =
      bounded_integer<unsigned, pucch_constants::f1::NOF_SYMS.start(), pucch_constants::f1::NOF_SYMS.stop()>;

  /// Number of OFDM symbols.
  nof_symbols nof_syms{pucch_constants::f1::NOF_SYMS.stop()};
  bool        intraslot_freq_hopping{false};
  /// Number of Initial Cyclic Shifts to use.
  pucch_nof_cyclic_shifts nof_cyc_shifts{pucch_nof_cyclic_shifts::no_cyclic_shift};
  /// Whether to configure the resources with different OCCs.
  bool occ_supported{false};
};

/// Parameters for the generation of PUCCH Format 2 resources.
struct pucch_f2_params {
  using nof_symbols =
      bounded_integer<unsigned, pucch_constants::f2::NOF_SYMS.start(), pucch_constants::f2::NOF_SYMS.stop()>;
  using nof_rbs = bounded_integer<unsigned, pucch_constants::f2::NOF_RBS.start(), pucch_constants::f2::NOF_RBS.stop()>;

  /// \brief Number of OFDM symbols.
  ///
  /// \remark For intraslot-freq-hopping, \c nof_symbols must be set to 2.
  nof_symbols nof_syms{pucch_constants::f2::NOF_SYMS.stop()};
  /// Number of RBs.
  nof_rbs max_nof_rbs{pucch_constants::f2::NOF_RBS.start()};
  /// Whether to configure intraslot frequency hopping.
  bool intraslot_freq_hopping{false};
  /// \brief Maximum payload in bits to be carried.
  ///
  /// \remark When this field is set, \c max_nof_rbs is ignored and the maximum number of RBs is computed according to
  ///         \ref get_pucch_format2_max_nof_prbs.
  std::optional<unsigned> max_payload_bits;
  /// Maximum allowed effective code rate.
  max_pucch_code_rate max_code_rate{max_pucch_code_rate::dot_25};
};

/// Collects the parameters for PUCCH Format 3 that can be configured.
struct pucch_f3_params {
  using nof_symbols =
      bounded_integer<unsigned, pucch_constants::f3::NOF_SYMS.start(), pucch_constants::f3::NOF_SYMS.stop()>;
  using nof_rbs = bounded_integer<unsigned, pucch_constants::f3::NOF_RBS.start(), pucch_constants::f3::NOF_RBS.stop()>;

  /// Number of OFDM symbols.
  nof_symbols nof_syms{pucch_constants::f3::NOF_SYMS.start()};
  /// Number of RBs.
  nof_rbs max_nof_rbs{pucch_constants::f3::NOF_RBS.start()};
  /// Whether to configure intraslot frequency hopping.
  bool intraslot_freq_hopping{false};
  /// \brief Maximum payload in bits to be carried.
  ///
  /// \remark When this field is set, \c max_nof_rbs is ignored and the maximum number of RBs is computed according to
  ///         \ref get_pucch_format3_max_nof_prbs.
  std::optional<unsigned> max_payload_bits;
  /// Maximum allowed effective code rate.
  max_pucch_code_rate max_code_rate{max_pucch_code_rate::dot_25};
  /// Whether to configure the resources with additional DM-RS symbols.
  bool additional_dmrs{false};
  /// Whether to configure the resources with pi/2-BPSK modulation.
  bool pi2_bpsk{false};
};

/// Collects the parameters for PUCCH Format 4 that can be configured.
struct pucch_f4_params {
  using nof_symbols =
      bounded_integer<unsigned, pucch_constants::f4::NOF_SYMS.start(), pucch_constants::f4::NOF_SYMS.stop()>;

  /// Number of OFDM symbols.
  nof_symbols nof_syms{pucch_constants::f4::NOF_SYMS.stop()};
  /// Whether to configure intraslot frequency hopping.
  bool intraslot_freq_hopping{false};
  /// Maximum allowed effective code rate.
  max_pucch_code_rate max_code_rate{max_pucch_code_rate::dot_25};
  /// Whether to configure the resources with additional DM-RS symbols.
  bool additional_dmrs{false};
  /// Whether to configure the resources with pi/2-BPSK modulation.
  bool pi2_bpsk{false};
  /// Whether to configure the resources with different OCCs.
  bool occ_supported{false};
  /// OCC length to use for PUCCH Format 4 resources.
  pucch_f4_occ_len occ_length{pucch_f4_occ_len::n2};
};

// Strong types for UCI specific PUCCH resource IDs.
struct pucch_resource_set_config_id_tag;
using pucch_resource_set_config_id = strong_type<uint8_t, struct pucch_res_set_cfg_id_tag, strong_equality>;
struct pucch_sr_resource_id_tag;
using pucch_sr_resource_id = strong_type<uint8_t, struct pucch_sr_resource_id_tag, strong_equality>;
struct pucch_csi_resource_id_tag;
using pucch_csi_resource_id = strong_type<uint8_t, struct pucch_csi_resource_id_tag, strong_equality>;

/// \brief Parameters for PUCCH configuration.
/// Defines the parameters that are used for the PUCCH configuration builder. These parameters are used to define the
/// number of PUCCH resources, as well as the PUCCH format-specific parameters.
struct pucch_resource_builder_params {
  static constexpr unsigned max_res_set_size = 8;
  using resource_set_size                    = bounded_integer<unsigned, 1, max_res_set_size>;

  /// Number of resources to use for Resource Set ID 0.
  resource_set_size res_set_0_size = 6;
  /// Number of resources to use for Resource Set ID 1.
  resource_set_size res_set_1_size = 6;
  /// \brief Number of separate PUCCH resource set configurations for HARQ-ACK reporting that are available in a cell.
  ///
  /// \remark Each resource set configuration defines different resources for Resource Set ID 0 and 1.
  /// \remark UEs will be distributed possibly over different configurations. The more configurations, the fewer UEs
  ///         will have to share the same set, reducing the chance that UEs won't be allocated PUCCH due to lack of
  ///         resources. However, the usage of PUCCH-dedicated REs will be proportional to the number of sets.
  unsigned nof_cell_res_set_configs = 1;
  /// \brief Defines how many PUCCH F0/F1 resources should be dedicated for SR at cell level.
  /// Each UE will be allocated 1 resource for SR.
  unsigned nof_cell_sr_resources = 2;
  /// \brief Defines how many PUCCH F2/F3/F4 resources should be dedicated for CSI at cell level.
  /// Each UE will be allocated 1 resource for CSI.
  unsigned nof_cell_csi_resources = 1;
  /// \brief Parameters for the generation of PUCCH Format 0 or Format 1 resources.
  ///
  /// \remark Having \c pucch_f1_params first forces the variant to use the Format 1 in the default constructor.
  std::variant<pucch_f1_params, pucch_f0_params> f0_or_f1_params;
  /// Parameters for the generation of PUCCH Format 2, Format 3 or Format 4 resources.
  std::variant<pucch_f2_params, pucch_f3_params, pucch_f4_params> f2_or_f3_or_f4_params;
  /// Maximum number of symbols per UL slot dedicated for PUCCH.
  /// \remark In case of Sounding Reference Signals (SRS) being used, the number of symbols should be reduced so that
  ///         the PUCCH resources do not overlap in symbols with the SRS resources.
  /// \remark This parameter should be computed by the GNB and not exposed to the user configuration interface.
  bounded_integer<unsigned, 1, 14> max_nof_symbols = NOF_OFDM_SYM_PER_SLOT_NORMAL_CP;

  // \brief Get the position of a given Resource Set ID 0 resource in the cell PUCCH resource list.
  //
  // \param res_set_cfg_id The resource set config index.
  // \param pri The index of the resource within the resource set.
  // \return The index of the PUCCH resource in the cell PUCCH resource list.
  unsigned get_res_set_0_cell_res_idx(pucch_resource_set_config_id res_set_cfg_id, unsigned pri) const
  {
    ocudu_assert(res_set_cfg_id.value() < nof_cell_res_set_configs,
                 "Resource set config index={} exceeds configured number of resource set configs={}",
                 res_set_cfg_id.value(),
                 nof_cell_res_set_configs);
    ocudu_assert(pri < res_set_0_size.value(),
                 "Resource index={} exceeds configured resource set size={}",
                 pri,
                 res_set_0_size.value());
    return res_set_cfg_id.value() * res_set_0_size.value() + pri;
  }

  // \brief Get the position of a given Resource Set ID 1 resource in the cell PUCCH resource list.
  //
  // \param res_set_cfg_id The resource set config index.
  // \param pri The index of the resource within the resource set.
  // \return The index of the PUCCH resource in the cell PUCCH resource list.
  unsigned get_res_set_1_cell_res_idx(pucch_resource_set_config_id res_set_cfg_id, unsigned pri) const
  {
    ocudu_assert(res_set_cfg_id.value() < nof_cell_res_set_configs,
                 "Resource set config index={} exceeds configured number of resource set configs={}",
                 res_set_cfg_id.value(),
                 nof_cell_res_set_configs);
    ocudu_assert(pri < res_set_1_size.value(),
                 "Resource index={} exceeds configured resource set size={}",
                 pri,
                 res_set_1_size.value());
    return nof_cell_res_set_configs * res_set_0_size.value() + nof_cell_sr_resources +
           res_set_cfg_id.value() * res_set_1_size.value() + pri;
  }

  // \brief Get the position of a given PUCCH resource for SR in the cell PUCCH resource list.
  //
  // \param sr_res_id The SR PUCCH resource index.
  // \return The index of the PUCCH resource in the cell PUCCH resource list.
  unsigned get_sr_cell_res_idx(pucch_sr_resource_id sr_res_id) const
  {
    ocudu_assert(sr_res_id.value() < nof_cell_sr_resources,
                 "SR resource index={} exceeds configured number of SR resources={}",
                 sr_res_id.value(),
                 nof_cell_sr_resources);
    return nof_cell_res_set_configs * res_set_0_size.value() + sr_res_id.value();
  }

  // \brief Get the position of a given PUCCH resource for CSI in the cell PUCCH resource list.
  //
  // \param csi_res_id The CSI PUCCH resource index.
  // \return The index of the PUCCH resource in the cell PUCCH resource list.
  unsigned get_csi_cell_res_idx(pucch_csi_resource_id csi_res_id) const
  {
    ocudu_assert(csi_res_id.value() < nof_cell_csi_resources,
                 "CSI resource index={} exceeds configured number of CSI resources={}",
                 csi_res_id.value(),
                 nof_cell_csi_resources);
    return nof_cell_res_set_configs * res_set_0_size.value() + nof_cell_sr_resources +
           nof_cell_res_set_configs * res_set_1_size.value() + csi_res_id.value();
  }
};

} // namespace ocudu
