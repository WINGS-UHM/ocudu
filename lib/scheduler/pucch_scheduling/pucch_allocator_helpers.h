
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

#include "../config/ue_configuration.h"
#include "ocudu/scheduler/result/pucch_info.h"

namespace ocudu {

/// Retrieves the correct N_{ID}^0 parameter for PUCCH scrambling from the UE configuration.
unsigned get_n_id0_scrambling(const ue_cell_configuration& ue_cell_cfg, unsigned cell_pci);

/// Contains the existing PUCCH grants currently allocated to a given UE.
class pucch_existing_pdus_handler
{
public:
  pucch_existing_pdus_handler(rnti_t crnti, span<pucch_info> pucchs, const pucch_config& pucch_cfg);

  [[nodiscard]] bool     is_empty() const { return pdus_cnt == 0; }
  [[nodiscard]] unsigned get_nof_unallocated_pdu() const { return pdus_cnt; }
  pucch_info*            get_next_pdu(static_vector<pucch_info, MAX_PUCCH_PDUS_PER_SLOT>& pucchs);
  void remove_unused_pdus(static_vector<pucch_info, MAX_PUCCH_PDUS_PER_SLOT>& pucchs, rnti_t rnti) const;
  void update_sr_pdu_bits(sr_nof_bits sr_bits, unsigned harq_ack_bits);
  void update_csi_pdu_bits(unsigned csi_part1_bits, sr_nof_bits sr_bits);
  void update_harq_pdu_bits(unsigned                                       harq_ack_bits,
                            sr_nof_bits                                    sr_bits,
                            unsigned                                       csi_part1_bits,
                            const pucch_resource&                          pucch_res_cfg,
                            const std::optional<pucch_common_all_formats>& common_params);

  pucch_info* sr_pdu{nullptr};
  pucch_info* harq_pdu{nullptr};
  pucch_info* csi_pdu{nullptr};

private:
  unsigned pdus_cnt = 0;
  unsigned pdu_id   = 0;
};

} // namespace ocudu
