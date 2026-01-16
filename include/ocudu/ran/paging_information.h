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

#include "ocudu/ran/du_types.h"
#include "ocudu/ran/time/radio_frame.h"

namespace ocudu {

/// Type of Paging. RAN initiated or CN initiated.
enum class paging_identity_type { ran_ue_paging_identity, cn_ue_paging_identity };

/// Description of eDRX paging information.
struct paging_edrx_information {
  /// Number of radio frames of the eDRX cycle in idle.
  radio_frames cycle_idle;
  /// Paging time window (PTW) length for eDRX.
  std::optional<std::chrono::seconds> ptw_len;
  /// \brief UE_ID of 16 bits specified in TS 38.413 and TS 38.473, and used for eDRX as per TS 38.304.
  /// As per TS 38.304, 12 bits are used for PF and PO calculation and 13/15 bits are used for subgrouping.
  unsigned extended_ue_identity_index_value;
  /// \brief UE_ID_H of 13 bits specified in TS 38.413 and TS 38.473, and used for eDRX as per TS 38.304.
  unsigned hashed_ue_identity_index_value;
};

struct paging_information {
  /// Cells at which to perform Paging of UE.
  std::vector<du_cell_index_t> paging_cells;
  paging_identity_type         paging_type_indicator;
  /// Paging identity assigned to UE. Possible values are \c I-RNTI-Value (Bit string of size 40) and \c NG-5G-S-TMSI
  /// (Bit string of size 48). See TS 38.331.
  uint64_t paging_identity;
  /// UE_ID: 5G-S-TMSI mod 1024. Used by the paging scheduler to calculate the Paging Frame.
  /// \remark See TS 38.304, clause 7.1.
  unsigned ue_identity_index_value;
  /// Paging DRX cycle specified by upper layers.
  std::optional<radio_frames> paging_drx;
  /// Paging eDRX information, if available.
  std::optional<paging_edrx_information> edrx;
  /// Values {1,...,8}. Lower value indicates higher priority.
  std::optional<unsigned> paging_priority;
  bool                    is_paging_origin_non_3gpp_access{false};
};

} // namespace ocudu
