/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "pucch_allocator_helpers.h"
#include "ocudu/ran/pucch/pucch_configuration.h"
#include "ocudu/ran/pucch/pucch_info.h"

using namespace ocudu;

unsigned ocudu::get_n_id0_scrambling(const ue_cell_configuration& ue_cell_cfg, unsigned cell_pci)
{
  // As per TS 38.211, Section 6.4.1.3.2.1, N_{ID}^0 is given by the higher-layer parameter scramblingID0 in the
  // DMRS-UplinkConfig IE if provided and by N_{ID}^{cell} otherwise. If a UE is configured with both
  // dmrs-UplinkForPUSCH-MappingTypeA and dmrs-UplinkForPUSCH-MappingTypeB, scramblingID0 is obtained from
  // dmrs-UplinkForPUSCH-MappingTypeB.

  // Check \c scrambling_id0 in \c dmrs-UplinkForPUSCH-MappingTypeB, first
  const auto& pusch_cfg = *ue_cell_cfg.init_bwp().ul_ded->pusch_cfg;
  if (pusch_cfg.pusch_mapping_type_b_dmrs.has_value() and
      pusch_cfg.pusch_mapping_type_b_dmrs.value().trans_precoder_disabled.has_value() and
      pusch_cfg.pusch_mapping_type_b_dmrs.value().trans_precoder_disabled.value().scrambling_id0.has_value()) {
    return pusch_cfg.pusch_mapping_type_b_dmrs.value().trans_precoder_disabled.value().scrambling_id0.value();
  }
  // Else, check \c scrambling_id0 in \c dmrs-UplinkForPUSCH-MappingTypeA.
  if (pusch_cfg.pusch_mapping_type_a_dmrs.has_value() and
      pusch_cfg.pusch_mapping_type_a_dmrs.value().trans_precoder_disabled.has_value() and
      pusch_cfg.pusch_mapping_type_a_dmrs.value().trans_precoder_disabled.value().scrambling_id0.has_value()) {
    return pusch_cfg.pusch_mapping_type_a_dmrs.value().trans_precoder_disabled.value().scrambling_id0.value();
  }

  return cell_pci;
}

// Checks if a PUCCH PDU is for SR.
static bool sr_id_match(const pucch_resource& pucch_res_cfg_lhs, const pucch_info& rhs)
{
  const bool prb_match =
      pucch_res_cfg_lhs.starting_prb == rhs.resources.prbs.start() and
      ((not pucch_res_cfg_lhs.second_hop_prb.has_value() and rhs.resources.second_hop_prbs.empty()) or
       (pucch_res_cfg_lhs.second_hop_prb.has_value() and pucch_res_cfg_lhs.second_hop_prb.value() and
        pucch_res_cfg_lhs.second_hop_prb == rhs.resources.second_hop_prbs.start()));
  const bool symb_match = pucch_res_cfg_lhs.starting_sym_idx == rhs.resources.symbols.start() and
                          pucch_res_cfg_lhs.nof_symbols == rhs.resources.symbols.length();
  const auto& f1_cfg       = std::get<pucch_format_1_cfg>(pucch_res_cfg_lhs.format_params);
  const auto& rhs_format_1 = std::get<pucch_format_1>(rhs.format_params);
  return prb_match && symb_match && f1_cfg.initial_cyclic_shift == rhs_format_1.initial_cyclic_shift &&
         f1_cfg.time_domain_occ == rhs_format_1.time_domain_occ;
}

static const pucch_resource* get_sr_pucch_resource(const pucch_config& pucch_cfg)
{
  const auto&    pucch_res_list  = pucch_cfg.pucch_res_list;
  const unsigned sr_pucch_res_id = pucch_cfg.sr_res_list[0].pucch_res_id.cell_res_id;
  // Search for the PUCCH resource with the correct PUCCH resource ID from the PUCCH resource list.
  const auto* res_cfg =
      std::find_if(pucch_res_list.begin(), pucch_res_list.end(), [sr_pucch_res_id](const pucch_resource& res) {
        return res.res_id.cell_res_id == sr_pucch_res_id;
      });

  return res_cfg != pucch_res_list.end() ? const_cast<pucch_resource*>(res_cfg) : nullptr;
}

pucch_existing_pdus_handler::pucch_existing_pdus_handler(rnti_t              crnti,
                                                         span<pucch_info>    pucchs,
                                                         const pucch_config& pucch_cfg)
{
  pdu_id = 0;

  for (auto& pucch : pucchs) {
    if (pucch.crnti == crnti and not pucch.pdu_context.is_common) {
      pucch.pdu_context.id = MAX_PUCCH_PDUS_PER_SLOT;

      if (pucch.format() == pucch_format::FORMAT_0) {
        // With Format 0, when there are both HARQ bits and SR bits in the same PDU (this means that the PUCCH HARQ
        // resource and SR resource have overlapping symbols), we only use the HARQ-ACK resource; the only case when
        // the SR PUCCH F0 is used is when there are only SR bits.
        if (pucch.uci_bits.sr_bits != sr_nof_bits::no_sr and pucch.uci_bits.harq_ack_nof_bits == 0U) {
          sr_pdu = &pucch;
          ++pdus_cnt;
        } else if (pucch.uci_bits.harq_ack_nof_bits != 0U and pucch.uci_bits.harq_ack_nof_bits <= 2U) {
          harq_pdu = &pucch;
          ++pdus_cnt;
        } else {
          ocudu_assertion_failure("Invalid HARQ/SR bits for PUCCH Format 0");
        }
      } else if (pucch.format() == pucch_format::FORMAT_1) {
        const pucch_resource* sr_res = get_sr_pucch_resource(pucch_cfg);
        if (pucch.uci_bits.sr_bits == sr_nof_bits::one and sr_res != nullptr and sr_id_match(*sr_res, pucch)) {
          sr_pdu = &pucch;
        } else {
          harq_pdu = &pucch;
        }
        ++pdus_cnt;
      } else {
        // Formats 2, 3 and 4.
        if (pucch.uci_bits.csi_part1_nof_bits != 0U and pucch.uci_bits.harq_ack_nof_bits == 0U) {
          csi_pdu = &pucch;
        } else {
          harq_pdu = &pucch;
        }
        ++pdus_cnt;
      }
    }
  }
}

pucch_info* pucch_existing_pdus_handler::get_next_pdu(static_vector<pucch_info, MAX_PUCCH_PDUS_PER_SLOT>& pucchs)
{
  if (is_empty()) {
    ocudu_assert(not pucchs.full(), "PUCCH grants list is full");
    auto* new_pdu           = &pucchs.emplace_back();
    new_pdu->pdu_context.id = pdu_id++;
    return new_pdu;
  }
  pucch_info* ret_grant = nullptr;
  if (csi_pdu != nullptr) {
    ret_grant                 = csi_pdu;
    ret_grant->pdu_context.id = pdu_id++;
    --pdus_cnt;
  } else if (sr_pdu != nullptr) {
    ret_grant                 = sr_pdu;
    ret_grant->pdu_context.id = pdu_id++;
    --pdus_cnt;
  } else if (harq_pdu != nullptr) {
    ret_grant                 = harq_pdu;
    ret_grant->pdu_context.id = pdu_id++;
    --pdus_cnt;
  }
  // NOTE: this cannot be nullptr, otherwise the function would have exited at the previous return.
  return ret_grant;
}

void pucch_existing_pdus_handler::remove_unused_pdus(static_vector<pucch_info, MAX_PUCCH_PDUS_PER_SLOT>& pucchs,
                                                     rnti_t                                              rnti) const
{
  if (pdus_cnt == 0) {
    return;
  }
  auto* it = pucchs.begin();
  while (it != pucchs.end()) {
    if (it->crnti == rnti and not it->pdu_context.is_common and it->pdu_context.id >= MAX_PUCCH_PDUS_PER_SLOT) {
      it = pucchs.erase(it);
    } else {
      ++it;
    }
  }
}

void pucch_existing_pdus_handler::update_sr_pdu_bits(sr_nof_bits sr_bits, unsigned harq_ack_bits)
{
  if (sr_pdu == nullptr) {
    return;
  }
  ocudu_assert(sr_pdu->format() == pucch_format::FORMAT_0 or sr_pdu->format() == pucch_format::FORMAT_1,
               "Only PUCCH Formats 0 or 1 can be used for SR grant");

  sr_pdu->uci_bits.sr_bits           = sr_bits;
  sr_pdu->uci_bits.harq_ack_nof_bits = harq_ack_bits;
  sr_pdu->pdu_context.id             = pdu_id++;

  // Once the grant is updated, set the pointer to null, as we don't want to process this again.
  sr_pdu = nullptr;
  --pdus_cnt;
}

void pucch_existing_pdus_handler::update_csi_pdu_bits(unsigned csi_part1_bits, sr_nof_bits sr_bits)
{
  ocudu_assert(csi_pdu->format() == pucch_format::FORMAT_2 or csi_pdu->format() == pucch_format::FORMAT_3 or
                   csi_pdu->format() == pucch_format::FORMAT_4,
               "Only PUCCH Formats 2, 3 and 4 can be used for CSI grant");

  csi_pdu->uci_bits.csi_part1_nof_bits = csi_part1_bits;
  csi_pdu->uci_bits.sr_bits            = sr_bits;
  csi_pdu->pdu_context.id              = pdu_id++;

  // Once the grant is updated, set the pointer to null, as we don't want to process this again.
  csi_pdu = nullptr;
  --pdus_cnt;
}

void pucch_existing_pdus_handler::update_harq_pdu_bits(unsigned                                       harq_ack_bits,
                                                       sr_nof_bits                                    sr_bits,
                                                       unsigned                                       csi_part1_bits,
                                                       const pucch_resource&                          pucch_res_cfg,
                                                       const std::optional<pucch_common_all_formats>& common_params)
{
  switch (harq_pdu->format()) {
    case pucch_format::FORMAT_2: {
      harq_pdu->uci_bits.csi_part1_nof_bits = csi_part1_bits;
      // After updating the UCI bits, we need to recompute the number of PRBs for PUCCH format 2, as per TS 38.213,
      // Section 9.2.5.2.
      const auto&    f2_cfg = std::get<pucch_format_2_3_cfg>(pucch_res_cfg.format_params);
      const unsigned nof_prbs =
          get_pucch_format2_nof_prbs(harq_ack_bits + sr_nof_bits_to_uint(sr_bits) + csi_part1_bits,
                                     f2_cfg.nof_prbs,
                                     pucch_res_cfg.nof_symbols,
                                     to_max_code_rate_float(common_params->max_c_rate));
      harq_pdu->resources.prbs.set(pucch_res_cfg.starting_prb, pucch_res_cfg.starting_prb + nof_prbs);
      if (pucch_res_cfg.second_hop_prb.has_value()) {
        harq_pdu->resources.second_hop_prbs.set(pucch_res_cfg.second_hop_prb.value(),
                                                pucch_res_cfg.second_hop_prb.value() + nof_prbs);
      }
    } break;
    case pucch_format::FORMAT_3: {
      harq_pdu->uci_bits.csi_part1_nof_bits = csi_part1_bits;
      // After updating the UCI bits, we need to recompute the number of PRBs for PUCCH format 3, as per TS 38.213,
      // Section 9.2.5.2.
      const auto&    f3_cfg = std::get<pucch_format_2_3_cfg>(pucch_res_cfg.format_params);
      const unsigned nof_prbs =
          get_pucch_format3_nof_prbs(harq_ack_bits + sr_nof_bits_to_uint(sr_bits) + csi_part1_bits,
                                     f3_cfg.nof_prbs,
                                     pucch_res_cfg.nof_symbols,
                                     to_max_code_rate_float(common_params->max_c_rate),
                                     pucch_res_cfg.second_hop_prb.has_value(),
                                     common_params->additional_dmrs,
                                     common_params->pi_2_bpsk);
      harq_pdu->resources.prbs.set(pucch_res_cfg.starting_prb, pucch_res_cfg.starting_prb + nof_prbs);
      if (pucch_res_cfg.second_hop_prb.has_value()) {
        harq_pdu->resources.second_hop_prbs.set(pucch_res_cfg.second_hop_prb.value(),
                                                pucch_res_cfg.second_hop_prb.value() + nof_prbs);
      }
    } break;
    case pucch_format::FORMAT_4: {
      harq_pdu->uci_bits.csi_part1_nof_bits = csi_part1_bits;
    } break;
    default:
      break;
  }
  harq_pdu->uci_bits.harq_ack_nof_bits = harq_ack_bits;
  harq_pdu->uci_bits.sr_bits           = sr_bits;
  harq_pdu->pdu_context.id             = pdu_id++;

  // Once the grant is updated, set the pointer to null, as we don't want to process this again.
  harq_pdu = nullptr;
  --pdus_cnt;
}
