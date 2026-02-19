// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI

#include "ocudu/fapi/message_loggers.h"
#include "ocudu/fapi/common/error_indication.h"
#include "ocudu/fapi/p7/messages/crc_indication.h"
#include "ocudu/fapi/p7/messages/dl_tti_request.h"
#include "ocudu/fapi/p7/messages/rach_indication.h"
#include "ocudu/fapi/p7/messages/rx_data_indication.h"
#include "ocudu/fapi/p7/messages/slot_indication.h"
#include "ocudu/fapi/p7/messages/srs_indication.h"
#include "ocudu/fapi/p7/messages/tx_data_request.h"
#include "ocudu/fapi/p7/messages/uci_indication.h"
#include "ocudu/fapi/p7/messages/ul_dci_request.h"
#include "ocudu/fapi/p7/messages/ul_tti_request.h"
#include "ocudu/support/format/fmt_to_c_str.h"

using namespace ocudu;
using namespace fapi;

void ocudu::fapi::log_error_indication(const error_indication& msg, unsigned sector_id, ocudulog::basic_logger& logger)
{
  if (OCUDU_LIKELY(!logger.debug.enabled())) {
    return;
  }

  fmt::memory_buffer buffer;
  fmt::format_to(std::back_inserter(buffer),
                 "Sector#{}: Error.indication slot={} error_code={} msg_id={}",
                 sector_id,
                 *msg.slot,
                 fmt::underlying(msg.error_code),
                 fmt::underlying(msg.message_id));
  if (msg.error_code == error_code_id::out_of_sync && msg.expected_slot) {
    fmt::format_to(std::back_inserter(buffer), " expected_slot={}", *msg.expected_slot);
  }

  logger.debug("{}", to_c_str(buffer));
}

/// Converts the given FAPI CRC SINR to dB as per SCF-222 v4.0 section 3.4.8.
static float to_crc_ul_sinr(int sinr)
{
  return static_cast<float>(sinr) * 0.002F;
}

/// Converts the given FAPI CRC RSSI to dB as per SCF-222 v4.0 section 3.4.8.
static float to_crc_ul_rssi(unsigned rssi)
{
  return static_cast<float>(static_cast<int>(rssi) - 1280) * 0.1F;
}

/// Converts the given FAPI CRC RSRP to dB as per SCF-222 v4.0 section 3.4.8.
static float to_crc_ul_rsrp(unsigned rsrp)
{
  return static_cast<float>(static_cast<int>(rsrp) - 1280) * 0.1F;
}

/// Appends the timing advance value to the given buffer if there is a timing advance.
static void
append_time_advance(fmt::memory_buffer& buffer, std::optional<phy_time_unit> timing_advance, subcarrier_spacing scs)
{
  if (!timing_advance) {
    return;
  }

  fmt::format_to(
      std::back_inserter(buffer), " ta={} ({:.1f}ns)", timing_advance->to_Ta(scs), timing_advance->to_seconds() * 1e9);
}

void ocudu::fapi::log_crc_indication(const crc_indication& msg, unsigned sector_id, ocudulog::basic_logger& logger)
{
  if (OCUDU_LIKELY(!logger.debug.enabled())) {
    return;
  }

  fmt::memory_buffer buffer;
  fmt::format_to(std::back_inserter(buffer), "Sector#{}: CRC.indication slot={}", sector_id, msg.slot);

  for (const auto& pdu : msg.pdus) {
    fmt::format_to(std::back_inserter(buffer),
                   "\n\t- CRC rnti={} harq_id={} tb_status={}",
                   pdu.rnti,
                   fmt::underlying(pdu.harq_id),
                   pdu.tb_crc_status_ok ? "OK" : "KO");
    append_time_advance(buffer, pdu.timing_advance_offset, msg.slot.scs());
    if (pdu.ul_sinr_metric != std::numeric_limits<decltype(pdu.ul_sinr_metric)>::min()) {
      fmt::format_to(std::back_inserter(buffer), " sinr={:.1f}", to_crc_ul_sinr(pdu.ul_sinr_metric));
    }
    if (pdu.rssi != std::numeric_limits<decltype(pdu.rssi)>::max()) {
      fmt::format_to(std::back_inserter(buffer), " rssi={:.1f}", to_crc_ul_rssi(pdu.rssi));
    }
    if (pdu.rsrp != std::numeric_limits<decltype(pdu.rsrp)>::max()) {
      fmt::format_to(std::back_inserter(buffer), " rsrp={:.1f}", to_crc_ul_rsrp(pdu.rsrp));
    }
  }

  logger.debug("{}", to_c_str(buffer));
}

/// Converts the given FAPI CRC RSRP to dB as per SCF-222 v4.0 section 3.4.8.
static float to_power_sss_ul_dci(float power_offset)
{
  return power_offset * 0.001;
}

static void log_dl_dci_pdu(const dl_dci_pdu& dci_pdu, fmt::memory_buffer& buffer)
{
  fmt::format_to(std::back_inserter(buffer),
                 " DCI rnti={} nid_pdcch_data={} nid_pdcch_dmrs={} nrnti_pdcch_data={} cce_index={} "
                 "dci_aggregation_level={} payload_size={}",
                 dci_pdu.rnti,
                 dci_pdu.nid_pdcch_data,
                 dci_pdu.nid_pdcch_dmrs,
                 dci_pdu.nrnti_pdcch_data,
                 dci_pdu.cce_index,
                 fmt::underlying(dci_pdu.dci_aggregation_level),
                 dci_pdu.payload.size());

  if (const auto* power_profile_nr_in_pdu = std::get_if<dl_dci_pdu::power_profile_nr>(&dci_pdu.power_config)) {
    fmt::format_to(
        std::back_inserter(buffer), " power_control_offset_ss={}", power_profile_nr_in_pdu->power_control_offset_ss);
  } else if (const auto* power_profile_sss_in_pdu = std::get_if<dl_dci_pdu::power_profile_sss>(&dci_pdu.power_config)) {
    fmt::format_to(std::back_inserter(buffer),
                   " dmrs_power_offset_db={} data_power_offset_db={}",
                   to_power_sss_ul_dci(power_profile_sss_in_pdu->dmrs_power_offset_db),
                   to_power_sss_ul_dci(power_profile_sss_in_pdu->data_power_offset_db));
  }

  if (dci_pdu.context.has_value()) {
    fmt::format_to(std::back_inserter(buffer), " with context");
  }
}

static void log_pdcch_pdu(const dl_pdcch_pdu& pdu, fmt::memory_buffer& buffer)
{
  fmt::format_to(std::back_inserter(buffer),
                 "\n\t- PDCCH bwp={} symb={} freq_domain_resource={} precoder_granularity={}",
                 pdu.coreset_bwp,
                 pdu.symbols,
                 pdu.freq_domain_resource,
                 fmt::underlying(pdu.precoder_granularity));

  log_dl_dci_pdu(pdu.dl_dci, buffer);

  if (const auto* dci_coreset_0 = std::get_if<fapi::dl_pdcch_pdu::mapping_coreset_0>(&pdu.mapping)) {
    fmt::format_to(std::back_inserter(buffer),
                   " CORESET0 reg_bundle_sz={} interleaver_sz={} shift_index={}",
                   dci_coreset_0->interleaved.reg_bundle_sz,
                   dci_coreset_0->interleaved.interleaver_sz,
                   dci_coreset_0->interleaved.shift_index);

  } else if (const auto* dci_interleaved = std::get_if<fapi::dl_pdcch_pdu::mapping_interleaved>(&pdu.mapping)) {
    fmt::format_to(std::back_inserter(buffer),
                   " INTERLEAVED reg_bundle_sz={} interleaver_sz={} shift_index={}",
                   dci_interleaved->interleaved.reg_bundle_sz,
                   dci_interleaved->interleaved.interleaver_sz,
                   dci_interleaved->interleaved.shift_index);

  } else if (const auto* dci_non_interleaved = std::get_if<fapi::dl_pdcch_pdu::mapping_non_interleaved>(&pdu.mapping)) {
    fmt::format_to(std::back_inserter(buffer), " NON INTERLEAVED reg_bundle_sz={}", dci_non_interleaved->reg_bundle_sz);
  }
}

static void log_ssb_pdu(const dl_ssb_pdu& pdu, fmt::memory_buffer& buffer)
{
  fmt::format_to(
      std::back_inserter(buffer),
      "\n\t- SSB pci={} beta_pss_profile={} ssb_block_index={} k_SSB={} pointA={} ssb_pattern_case={} scs={} L_max={}",
      pdu.phys_cell_id,
      fmt::underlying(pdu.beta_pss_profile_nr),
      fmt::underlying(pdu.ssb_block_index),
      pdu.subcarrier_offset.value(),
      pdu.ssb_offset_pointA.value(),
      to_string(pdu.case_type),
      to_string(pdu.scs),
      pdu.L_max);
}

static void log_pdsch_pdu(const dl_pdsch_pdu& pdu, fmt::memory_buffer& buffer)
{
  fmt::format_to(std::back_inserter(buffer),
                 "\n\t- PDSCH rnti={} bwp={} symb={} CW: tbs={} mod={} rv_idx={}",
                 pdu.rnti,
                 pdu.bwp,
                 pdu.symbols,
                 pdu.cws.front().tb_size,
                 pdu.cws.front().qam_mod_order,
                 pdu.cws.front().rv_index);
}

static void log_csi_rs_pdu(const dl_csi_rs_pdu& pdu, fmt::memory_buffer& buffer)
{
  if (pdu.type == csi_rs_type::CSI_RS_NZP) {
    fmt::format_to(std::back_inserter(buffer),
                   "\n\t- NZP-CSI-RS crbs={} type={} freq_domain={} row={} symbL0={} symbL1={} cdm_type={} "
                   "freq_density={} scramb_id={}",
                   pdu.crbs,
                   to_string(pdu.type),
                   pdu.freq_domain,
                   pdu.row,
                   pdu.symb_L0,
                   pdu.symb_L1,
                   to_string(pdu.cdm_type),
                   to_string(pdu.freq_density),
                   pdu.scramb_id);
    return;
  }

  if (pdu.type == csi_rs_type::CSI_RS_ZP) {
    fmt::format_to(std::back_inserter(buffer),
                   "\n\t- ZP-CSI-RS crbs={} row={} symbL0={} symbL1={}",
                   pdu.crbs,
                   pdu.row,
                   pdu.symb_L0,
                   pdu.symb_L1);
    return;
  }
}

static void log_prs_pdu(const dl_prs_pdu& pdu, fmt::memory_buffer& buffer)
{
  fmt::format_to(std::back_inserter(buffer),
                 "\n\t- PRS comb_size={} comb_offset={} symb={}:{} CRBs={} n_id={}",
                 fmt::underlying(pdu.comb_size),
                 pdu.comb_offset,
                 pdu.first_symbol,
                 fmt::underlying(pdu.num_symbols),
                 pdu.crbs,
                 pdu.nid_prs);
}

void ocudu::fapi::log_dl_tti_request(const dl_tti_request& msg, unsigned sector_id, ocudulog::basic_logger& logger)
{
  if (OCUDU_LIKELY(!logger.debug.enabled())) {
    return;
  }

  fmt::memory_buffer buffer;
  fmt::format_to(std::back_inserter(buffer), "Sector#{}: DL_TTI.request slot={}", sector_id, msg.slot);

  for (const auto& pdu : msg.pdus) {
    if (const auto* csi_rs_pdu = std::get_if<dl_csi_rs_pdu>(&pdu.pdu)) {
      log_csi_rs_pdu(*csi_rs_pdu, buffer);
      continue;
    }

    if (const auto* pdcch_pdu = std::get_if<dl_pdcch_pdu>(&pdu.pdu)) {
      log_pdcch_pdu(*pdcch_pdu, buffer);
      continue;
    }

    if (const auto* pdsch_pdu = std::get_if<dl_pdsch_pdu>(&pdu.pdu)) {
      log_pdsch_pdu(*pdsch_pdu, buffer);
      continue;
    }
    if (const auto* ssb_pdu = std::get_if<dl_ssb_pdu>(&pdu.pdu)) {
      log_ssb_pdu(*ssb_pdu, buffer);
      continue;
    }

    if (const auto* prs_pdu = std::get_if<dl_prs_pdu>(&pdu.pdu)) {
      log_prs_pdu(*prs_pdu, buffer);
      continue;
    }
  }

  logger.debug("{}", to_c_str(buffer));
}

/// Converts the given FAPI RACH occasion RSSI to dB as per SCF-222 v4.0 section 3.4.11.
static float to_rach_rssi_dB(int fapi_rssi)
{
  return (fapi_rssi - 140000) * 0.001F;
}

/// Converts the given FAPI RACH occasion SNR to dB as per SCF-222 v4.0 section 3.4.11.
static float to_rach_snr_dB(int fapi_snr)
{
  return (fapi_snr - 128) * 0.5F;
}

/// Converts the given FAPI RACH preamble power to dB as per SCF-222 v4.0 section 3.4.11.
static float to_rach_preamble_power_dB(int fapi_power)
{
  return static_cast<float>(fapi_power - 140000) * 0.001F;
}

/// Converts the given FAPI RACH preamble SNR to dB as per SCF-222 v4.0 section 3.4.11.
static float to_rach_preamble_snr_dB(int fapi_snr)
{
  return (fapi_snr - 128) * 0.5F;
}

void ocudu::fapi::log_rach_indication(const rach_indication& msg, unsigned sector_id, ocudulog::basic_logger& logger)
{
  if (OCUDU_LIKELY(!logger.debug.enabled())) {
    return;
  }

  fmt::memory_buffer buffer;
  fmt::format_to(std::back_inserter(buffer), "Sector#{}: RACH.indication slot={}", sector_id, msg.slot);

  for (const auto& pdu : msg.pdus) {
    fmt::format_to(std::back_inserter(buffer),
                   "\n\t- PRACH symb_idx={} slot_idx={} ra_index={}",
                   pdu.symbol_index,
                   pdu.slot_index,
                   pdu.ra_index);
    if (pdu.avg_rssi != std::numeric_limits<decltype(pdu.avg_rssi)>::max()) {
      fmt::format_to(std::back_inserter(buffer), " rssi={:.1f}", to_rach_rssi_dB(pdu.avg_rssi));
    }
    fmt::format_to(std::back_inserter(buffer), " avg_snr={:.1f}", to_rach_snr_dB(pdu.avg_snr));
    fmt::format_to(std::back_inserter(buffer), " nof_preambles={}:", pdu.preambles.size());

    // Log the preambles.
    for (const auto& preamble : pdu.preambles) {
      fmt::format_to(std::back_inserter(buffer), "\n\t\t- PREAMBLE index={}", preamble.preamble_index);
      append_time_advance(buffer, preamble.timing_advance_offset, msg.slot.scs());
      if (preamble.preamble_pwr != std::numeric_limits<decltype(preamble.preamble_pwr)>::max()) {
        fmt::format_to(std::back_inserter(buffer), " pwr={:.1f}", to_rach_preamble_power_dB(preamble.preamble_pwr));
      }
      if (preamble.preamble_snr != std::numeric_limits<decltype(preamble.preamble_snr)>::max()) {
        fmt::format_to(std::back_inserter(buffer), " snr={:.1f}", to_rach_preamble_snr_dB(preamble.preamble_snr));
      }
    }
  }

  logger.debug("{}", to_c_str(buffer));
}

void ocudu::fapi::log_rx_data_indication(const rx_data_indication& msg,
                                         unsigned                  sector_id,
                                         ocudulog::basic_logger&   logger)
{
  if (OCUDU_LIKELY(!logger.debug.enabled())) {
    return;
  }

  fmt::memory_buffer buffer;
  fmt::format_to(std::back_inserter(buffer), "Sector#{}: Rx_Data.indication slot={}", sector_id, msg.slot);

  for (const auto& pdu : msg.pdus) {
    fmt::format_to(std::back_inserter(buffer),
                   "\n\t- PDU rnti={} harq_id={} tbs={}",
                   pdu.rnti,
                   fmt::underlying(pdu.harq_id),
                   pdu.transport_block.size());
  }

  logger.debug("{}", to_c_str(buffer));
}

void ocudu::fapi::log_tx_data_request(const tx_data_request& msg, unsigned sector_id, ocudulog::basic_logger& logger)
{
  if (OCUDU_LIKELY(!logger.debug.enabled())) {
    return;
  }

  logger.debug("Sector#{}: Tx_Data.request slot={} nof_pdus={}", sector_id, msg.slot, msg.pdus.size());
}

/// Converts the given FAPI UCI SINR to dB as per SCF-222 v4.0 section 3.4.9.
static float to_uci_ul_sinr(int sinr)
{
  return static_cast<float>(sinr) * 0.002F;
}

/// Converts the given FAPI UCI RSRP to dB as per SCF-222 v4.0 section 3.4.9.
static float to_uci_ul_rsrp(unsigned rsrp)
{
  return static_cast<float>(static_cast<int>(rsrp) - 1280) * 0.1F;
}

/// Converts the given FAPI UCI RSSI to dB as per SCF-222 v4.0 section 3.4.9.
static float to_uci_ul_rssi(unsigned rssi)
{
  return static_cast<float>(static_cast<int>(rssi) - 1280) * 0.1F;
}

static void
log_uci_pucch_f0_f1_pdu(const uci_pucch_pdu_format_0_1& pdu, subcarrier_spacing scs, fmt::memory_buffer& buffer)
{
  fmt::format_to(std::back_inserter(buffer),
                 "\n\t- UCI PUCCH format 0/1 format={} rnti={}",
                 fmt::underlying(pdu.pucch_format),
                 pdu.rnti);

  if (pdu.ul_sinr_metric != std::numeric_limits<decltype(pdu.ul_sinr_metric)>::min()) {
    fmt::format_to(std::back_inserter(buffer), " sinr={:.1f}", to_uci_ul_sinr(pdu.ul_sinr_metric));
  }
  append_time_advance(buffer, pdu.timing_advance_offset, scs);

  if (pdu.rssi != std::numeric_limits<decltype(pdu.rssi)>::max()) {
    fmt::format_to(std::back_inserter(buffer), " rssi={:.1f}", to_uci_ul_rssi(pdu.rsrp));
  }

  if (pdu.rsrp != std::numeric_limits<decltype(pdu.rsrp)>::max()) {
    fmt::format_to(std::back_inserter(buffer), " rsrp={:.1f}", to_uci_ul_rsrp(pdu.rsrp));
  }

  if (pdu.sr.has_value()) {
    fmt::format_to(std::back_inserter(buffer), " SR: sr={}", pdu.sr->sr_detected ? "detected" : "not detected");
  }

  if (pdu.harq.has_value()) {
    fmt::format_to(std::back_inserter(buffer), " HARQ: harq_ack=");
    for (unsigned i = 0, e = pdu.harq->harq_values.size(), last = e - 1; i != e; ++i) {
      fmt::format_to(std::back_inserter(buffer), "{}", to_string(pdu.harq->harq_values[i]));
      if (i != last) {
        fmt::format_to(std::back_inserter(buffer), ",");
      }
    }
  }
}

static void
log_uci_pucch_f234_pdu(const uci_pucch_pdu_format_2_3_4& pdu, subcarrier_spacing scs, fmt::memory_buffer& buffer)
{
  fmt::format_to(std::back_inserter(buffer),
                 "\n\t- UCI PUCCH format 2/3/4 format={} rnti={}",
                 fmt::underlying(pdu.pucch_format) + 2,
                 pdu.rnti);

  if (pdu.ul_sinr_metric != std::numeric_limits<decltype(pdu.ul_sinr_metric)>::min()) {
    fmt::format_to(std::back_inserter(buffer), " sinr={:.1f}", to_uci_ul_sinr(pdu.ul_sinr_metric));
  }

  append_time_advance(buffer, pdu.timing_advance_offset, scs);

  if (pdu.rssi != std::numeric_limits<decltype(pdu.rssi)>::max()) {
    fmt::format_to(std::back_inserter(buffer), " rssi={:.1f}", to_uci_ul_rssi(pdu.rsrp));
  }

  if (pdu.rsrp != std::numeric_limits<decltype(pdu.rsrp)>::max()) {
    fmt::format_to(std::back_inserter(buffer), " rsrp={:.1f}", to_uci_ul_rsrp(pdu.rsrp));
  }

  if (pdu.sr.has_value()) {
    fmt::format_to(std::back_inserter(buffer), " SR: bit_len={}", pdu.sr->sr_payload.size());
  }
  if (pdu.harq.has_value()) {
    fmt::format_to(std::back_inserter(buffer),
                   " HARQ: detection={} bit_len={}",
                   fmt::underlying(pdu.harq->detection_status),
                   pdu.harq->expected_bit_length);
  }
  if (pdu.csi_part1.has_value()) {
    fmt::format_to(std::back_inserter(buffer),
                   " CSI1: detection={} bit_len={}",
                   fmt::underlying(pdu.csi_part1->detection_status),
                   pdu.csi_part1->expected_bit_length);
  }
  if (pdu.csi_part2.has_value()) {
    fmt::format_to(std::back_inserter(buffer),
                   " CSI2: detection={} bit_len={}",
                   fmt::underlying(pdu.csi_part2->detection_status),
                   pdu.csi_part2->expected_bit_length);
  }
}

static void log_uci_pusch_pdu(const uci_pusch_pdu& pdu, subcarrier_spacing scs, fmt::memory_buffer& buffer)
{
  fmt::format_to(std::back_inserter(buffer), "\n\t- UCI PUSCH rnti={}", pdu.rnti);

  if (pdu.ul_sinr_metric != std::numeric_limits<decltype(pdu.ul_sinr_metric)>::min()) {
    fmt::format_to(std::back_inserter(buffer), " sinr={:.1f}", to_uci_ul_sinr(pdu.ul_sinr_metric));
  }
  append_time_advance(buffer, pdu.timing_advance_offset, scs);

  if (pdu.rssi != std::numeric_limits<decltype(pdu.rssi)>::max()) {
    fmt::format_to(std::back_inserter(buffer), " rssi={:.1f}", to_uci_ul_rssi(pdu.rssi));
  }

  if (pdu.rsrp != std::numeric_limits<decltype(pdu.rsrp)>::max()) {
    fmt::format_to(std::back_inserter(buffer), " rsrp={:.1f}", to_uci_ul_rsrp(pdu.rsrp));
  }

  if (pdu.harq.has_value()) {
    fmt::format_to(std::back_inserter(buffer),
                   " HARQ: detection={} bit_len={}",
                   fmt::underlying(pdu.harq->detection_status),
                   pdu.harq->expected_bit_length);
  }
  if (pdu.csi_part1.has_value()) {
    fmt::format_to(std::back_inserter(buffer),
                   " CSI1: detection={} bit_len={}",
                   fmt::underlying(pdu.csi_part1->detection_status),
                   pdu.csi_part1->expected_bit_length);
  }
  if (pdu.csi_part2.has_value()) {
    fmt::format_to(std::back_inserter(buffer),
                   " CSI2: detection={} bit_len={}",
                   fmt::underlying(pdu.csi_part2->detection_status),
                   pdu.csi_part2->expected_bit_length);
  }
}

void ocudu::fapi::log_uci_indication(const uci_indication& msg, unsigned sector_id, ocudulog::basic_logger& logger)
{
  if (OCUDU_LIKELY(!logger.debug.enabled())) {
    return;
  }

  fmt::memory_buffer buffer;
  fmt::format_to(std::back_inserter(buffer), "Sector#{}: UCI.indication slot={}", sector_id, msg.slot);

  for (const auto& pdu : msg.pdus) {
    if (const auto* uci_pusch = std::get_if<fapi::uci_pusch_pdu>(&pdu)) {
      log_uci_pusch_pdu(*uci_pusch, msg.slot.scs(), buffer);
    } else if (const auto* uci_pusch_format_0_1 = std::get_if<fapi::uci_pucch_pdu_format_0_1>(&pdu)) {
      log_uci_pucch_f0_f1_pdu(*uci_pusch_format_0_1, msg.slot.scs(), buffer);
    } else if (const auto* uci_pusch_format_2_3_4 = std::get_if<fapi::uci_pucch_pdu_format_2_3_4>(&pdu)) {
      log_uci_pucch_f234_pdu(*uci_pusch_format_2_3_4, msg.slot.scs(), buffer);
    }
  }

  logger.debug("{}", to_c_str(buffer));
}

void ocudu::fapi::log_srs_indication(const srs_indication& msg, unsigned sector_id, ocudulog::basic_logger& logger)
{
  if (OCUDU_LIKELY(!logger.debug.enabled())) {
    return;
  }
  fmt::memory_buffer buffer;
  fmt::format_to(std::back_inserter(buffer), "Sector#{}: SRS.indication slot={}", sector_id, msg.slot);

  for (const auto& pdu : msg.pdus) {
    fmt::format_to(std::back_inserter(buffer), "\n\t-  rnti={}", pdu.rnti);
    append_time_advance(buffer, pdu.timing_advance_offset, msg.slot.scs());

    if (!pdu.positioning.has_value()) {
      continue;
    }

    if (pdu.positioning->ul_relative_toa) {
      fmt::format_to(std::back_inserter(buffer), " RTOA_s={}", *pdu.positioning->rsrp);
    }
    if (pdu.positioning->rsrp) {
      fmt::format_to(std::back_inserter(buffer), " RSRP={}", *pdu.positioning->rsrp);
    }
  }

  logger.debug("{}", to_c_str(buffer));
}

static void log_prach_pdu(const ul_prach_pdu& pdu, fmt::memory_buffer& buffer)
{
  fmt::format_to(std::back_inserter(buffer),
                 "\n\t- PRACH num_prach_ocas={} format={} fd_ra={}:{} symb={} z_corr={} preambles={}",
                 pdu.num_prach_ocas,
                 to_string(pdu.prach_format),
                 pdu.index_fd_ra,
                 pdu.num_fd_ra,
                 pdu.prach_start_symbol,
                 pdu.num_cs,
                 pdu.preambles);
}

static void log_pusch_pdu(const ul_pusch_pdu& pdu, fmt::memory_buffer& buffer)
{
  fmt::format_to(
      std::back_inserter(buffer),
      "\n\t- PUSCH rnti={} bwp={} scs={} cp={} target_code_rate={} modulation={} mcs_index={} "
      "mcs_table={} nid_pusch={} num_layers={} ul_dmrs_symb_pos={} dmrs_type={} pusch_dmrs_scrambling_id={} "
      "nscid={} dmrs_ports={} tx_direct_current_location={} symb={} ldpc_base_graph={} tb_size_lbrm_bytes={}",
      pdu.rnti,
      pdu.bwp,
      fmt::underlying(pdu.scs),
      pdu.cp,
      pdu.target_code_rate,
      to_string(pdu.qam_mod_order),
      pdu.mcs_index,
      pusch_mcs_table_to_string(pdu.mcs_table),
      pdu.nid_pusch,
      pdu.num_layers,
      pdu.ul_dmrs_symb_pos.to_uint64(),
      fmt::underlying(pdu.dmrs_type),
      pdu.pusch_dmrs_scrambling_id,
      pdu.nscid,
      pdu.dmrs_ports.to_uint64(),
      pdu.tx_direct_current_location,
      pdu.symbols,
      fmt::underlying(pdu.ldpc_base_graph),
      pdu.tb_size_lbrm_bytes.value());

  if (const auto* tp_enabled = std::get_if<ul_pusch_pdu::transform_precoding_enabled>(&pdu.transform_precoding)) {
    fmt::format_to(std::back_inserter(buffer),
                   " Transport Decoding Enabled pusch_dmrs_identity={}",
                   tp_enabled->pusch_dmrs_identity);
  } else if (const auto* tp_disabled =
                 std::get_if<ul_pusch_pdu::transform_precoding_disabled>(&pdu.transform_precoding)) {
    fmt::format_to(std::back_inserter(buffer),
                   " Transport Decoding Disabled num_dmrs_cdm_grps_no_data={}",
                   tp_disabled->num_dmrs_cdm_grps_no_data);
  }

  fmt::format_to(std::back_inserter(buffer),
                 " Resource Allocation Type 1 vrbs={}",
                 pdu.resource_allocation_1.vrbs,
                 pdu.resource_allocation_1.vrbs);

  if (pdu.pusch_data.has_value()) {
    fmt::format_to(std::back_inserter(buffer),
                   " CW: rv_idx={} harq_id={} new_data={} tbs={}",
                   pdu.pusch_data->rv_index,
                   fmt::underlying(pdu.pusch_data->harq_process_id),
                   pdu.pusch_data->new_data,
                   pdu.pusch_data->tb_size);
  }

  if (pdu.pusch_uci.has_value()) {
    fmt::format_to(std::back_inserter(buffer),
                   " UCI: harq_bit_len={} csi1_bit_len={} alpha_scaling={} beta_offset_harq_ack={} beta_offset_csi1={} "
                   "beta_offset_csi2={}",
                   pdu.pusch_uci->harq_ack_bit.value(),
                   pdu.pusch_uci->csi_part1_bit.value(),
                   alpha_scaling_to_float(pdu.pusch_uci->alpha_scaling),
                   pdu.pusch_uci->beta_offset_harq_ack,
                   pdu.pusch_uci->beta_offset_csi1,
                   pdu.pusch_uci->beta_offset_csi2);
  }
}

static void
log_pucch_format_0_pdu(const ul_pucch_pdu& pdu, const ul_pucch_pdu_format_0& format_0_pdu, fmt::memory_buffer& buffer)
{
  fmt::format_to(std::back_inserter(buffer),
                 "\n\t- PUCCH rnti={} bwp={} scs={} cp={} FORMAT_0 nid_pucch_hopping={} initial_cyclic_shift={} "
                 "sr_present={} prb={} symb={} harq_bit_len={}",
                 pdu.rnti,
                 pdu.bwp,
                 to_string(pdu.scs),
                 pdu.cp.to_string(),
                 format_0_pdu.nid_pucch_hopping,
                 format_0_pdu.initial_cyclic_shift,
                 format_0_pdu.sr_present,
                 pdu.prbs,
                 pdu.symbols,
                 format_0_pdu.bit_len_harq.value());

  if (pdu.second_hop_prb.has_value()) {
    fmt::format_to(std::back_inserter(buffer), " intra_slot_frequency_hopping={}", *pdu.second_hop_prb);
  }
}

static void
log_pucch_format_1_pdu(const ul_pucch_pdu& pdu, const ul_pucch_pdu_format_1& format_1_pdu, fmt::memory_buffer& buffer)
{
  fmt::format_to(std::back_inserter(buffer),
                 "\n\t- PUCCH rnti={} bwp={} scs={} cp={} FORMAT_1 nid_pucch_hopping={} initial_cyclic_shift={} "
                 "time_domain_occ_index={} sr_present={} prb={} symb={} harq_bit_len={}",
                 pdu.rnti,
                 pdu.bwp,
                 to_string(pdu.scs),
                 pdu.cp.to_string(),
                 format_1_pdu.nid_pucch_hopping,
                 format_1_pdu.initial_cyclic_shift,
                 format_1_pdu.time_domain_occ_index,
                 format_1_pdu.sr_present,
                 pdu.prbs,
                 pdu.symbols,
                 format_1_pdu.bit_len_harq.value());

  if (pdu.second_hop_prb.has_value()) {
    fmt::format_to(std::back_inserter(buffer), " intra_slot_frequency_hopping={}", *pdu.second_hop_prb);
  }
}

static void
log_pucch_format_2_pdu(const ul_pucch_pdu& pdu, const ul_pucch_pdu_format_2& format_2_pdu, fmt::memory_buffer& buffer)
{
  fmt::format_to(
      std::back_inserter(buffer),
      "\n\t- PUCCH rnti={} bwp={} scs={} cp={} FORMAT_2 nid_pucch_scrambling={} nid0_pucch_dmrs_scrambling={} "
      "sr_bit_len={} csi_part1_bit_length={} prb={} symb={} harq_bit_len={}",
      pdu.rnti,
      pdu.bwp,
      to_string(pdu.scs),
      pdu.cp.to_string(),
      format_2_pdu.nid_pucch_scrambling,
      format_2_pdu.nid0_pucch_dmrs_scrambling,
      fmt::underlying(format_2_pdu.sr_bit_len),
      format_2_pdu.csi_part1_bit_length.value(),
      pdu.prbs,
      pdu.symbols,
      format_2_pdu.bit_len_harq.value());

  if (pdu.second_hop_prb.has_value()) {
    fmt::format_to(std::back_inserter(buffer), " intra_slot_frequency_hopping={}", *pdu.second_hop_prb);
  }
}

static void
log_pucch_format_3_pdu(const ul_pucch_pdu& pdu, const ul_pucch_pdu_format_3& format_3_pdu, fmt::memory_buffer& buffer)
{
  fmt::format_to(std::back_inserter(buffer),
                 "\n\t- PUCCH rnti={} bwp={} scs={} cp={} FORMAT_3 nid_pucch_hopping={} nid_pucch_scrambling={} "
                 "add_dmrs_flag={} nid0_pucch_dmrs_scrambling={} m0_pucch_dmrs_cyclic_shift={} sr_bit_len={} "
                 "csi_part1_bit_length={} pi2_bpsk={} prb={} symb={} harq_bit_len={} ",
                 pdu.rnti,
                 pdu.bwp,
                 to_string(pdu.scs),
                 pdu.cp.to_string(),
                 format_3_pdu.nid_pucch_hopping,
                 format_3_pdu.nid_pucch_scrambling,
                 format_3_pdu.add_dmrs_flag,
                 format_3_pdu.nid0_pucch_dmrs_scrambling,
                 format_3_pdu.m0_pucch_dmrs_cyclic_shift,
                 fmt::underlying(format_3_pdu.sr_bit_len),
                 format_3_pdu.csi_part1_bit_length.value(),
                 format_3_pdu.pi2_bpsk,
                 pdu.prbs,
                 pdu.symbols,
                 format_3_pdu.bit_len_harq.value());

  if (pdu.second_hop_prb.has_value()) {
    fmt::format_to(std::back_inserter(buffer), " intra_slot_frequency_hopping={}", *pdu.second_hop_prb);
  }
}

static void
log_pucch_format_4_pdu(const ul_pucch_pdu& pdu, const ul_pucch_pdu_format_4& format_4_pdu, fmt::memory_buffer& buffer)
{
  fmt::format_to(
      std::back_inserter(buffer),
      "\n\t- PUCCH rnti={} bwp={} scs={} cp={} FORMAT_4 nid_pucch_hopping={} nid_pucch_scrambling={} "
      "pre_dft_occ_idx={} pre_dft_occ_len={} add_dmrs_flag={} nid0_pucch_dmrs_scrambling={} "
      "m0_pucch_dmrs_cyclic_shift={} sr_bit_len={} csi_part1_bit_length={} pi2_bpsk={} prb={} symb={} harq_bit_len={}",
      pdu.rnti,
      pdu.bwp,
      to_string(pdu.scs),
      pdu.cp.to_string(),
      format_4_pdu.nid_pucch_hopping,
      format_4_pdu.nid_pucch_scrambling,
      format_4_pdu.pre_dft_occ_idx,
      format_4_pdu.pre_dft_occ_len,
      format_4_pdu.add_dmrs_flag,
      format_4_pdu.nid0_pucch_dmrs_scrambling,
      format_4_pdu.m0_pucch_dmrs_cyclic_shift,
      fmt::underlying(format_4_pdu.sr_bit_len),
      format_4_pdu.csi_part1_bit_length.value(),
      format_4_pdu.pi2_bpsk,
      pdu.prbs,
      pdu.symbols,
      format_4_pdu.bit_len_harq.value());

  if (pdu.second_hop_prb.has_value()) {
    fmt::format_to(std::back_inserter(buffer), " intra_slot_frequency_hopping={}", *pdu.second_hop_prb);
  }
}

static void log_pucch_pdu(const ul_pucch_pdu& pdu, fmt::memory_buffer& buffer)
{
  if (const auto* format0 = std::get_if<ul_pucch_pdu_format_0>(&pdu.format)) {
    log_pucch_format_0_pdu(pdu, *format0, buffer);
    return;
  }

  if (const auto* format1 = std::get_if<ul_pucch_pdu_format_1>(&pdu.format)) {
    log_pucch_format_1_pdu(pdu, *format1, buffer);
  }

  if (const auto* format2 = std::get_if<ul_pucch_pdu_format_2>(&pdu.format)) {
    log_pucch_format_2_pdu(pdu, *format2, buffer);
    return;
  }
  if (const auto* format3 = std::get_if<ul_pucch_pdu_format_3>(&pdu.format)) {
    log_pucch_format_3_pdu(pdu, *format3, buffer);
    return;
  }

  if (const auto* format4 = std::get_if<ul_pucch_pdu_format_4>(&pdu.format)) {
    log_pucch_format_4_pdu(pdu, *format4, buffer);
    return;
  }
}

static void log_srs_pdu(const ul_srs_pdu& pdu, fmt::memory_buffer& buffer)
{
  fmt::format_to(
      std::back_inserter(buffer),
      "\n\t- SRS rnti={} bwp={} nof_ports={} symb={}:{} config_idx={} comb=(size={} offset={} cyclic_shift={}) "
      "freq_shift={} type={} normalized_channel_iq_matrix_req={} positioning_report_req={}",
      pdu.rnti,
      pdu.bwp,
      fmt::underlying(pdu.num_ant_ports),
      pdu.time_start_position,
      pdu.ofdm_symbols.length(),
      pdu.config_index,
      fmt::underlying(pdu.comb_size),
      pdu.comb_offset,
      pdu.cyclic_shift,
      pdu.frequency_shift,
      to_string(pdu.resource_type),
      pdu.enable_normalized_iq_matrix_report ? "enabled" : "disabled",
      pdu.enable_positioning_report ? "enabled" : "disabled");
}

void ocudu::fapi::log_ul_tti_request(const ul_tti_request& msg, unsigned sector_id, ocudulog::basic_logger& logger)
{
  if (OCUDU_LIKELY(!logger.debug.enabled())) {
    return;
  }

  fmt::memory_buffer buffer;
  fmt::format_to(std::back_inserter(buffer), "Sector#{}: UL_TTI.request slot={}", sector_id, msg.slot);

  for (const auto& pdu : msg.pdus) {
    if (const auto* prach_pdu = std::get_if<ul_prach_pdu>(&pdu.pdu)) {
      log_prach_pdu(*prach_pdu, buffer);
      continue;
    }

    if (const auto* pucch_pdu = std::get_if<ul_pucch_pdu>(&pdu.pdu)) {
      log_pucch_pdu(*pucch_pdu, buffer);
      continue;
    }

    if (const auto* pusch_pdu = std::get_if<ul_pusch_pdu>(&pdu.pdu)) {
      log_pusch_pdu(*pusch_pdu, buffer);
      continue;
    }

    if (const auto* srs_pdu = std::get_if<ul_srs_pdu>(&pdu.pdu)) {
      log_srs_pdu(*srs_pdu, buffer);
      continue;
    }

    ocudu_assert(0, "UL_TTI.request PDU type value not recognized.");
  }

  logger.debug("{}", to_c_str(buffer));
}

void ocudu::fapi::log_slot_indication(const slot_indication& msg, unsigned sector_id, ocudulog::basic_logger& logger)
{
  if (OCUDU_LIKELY(!logger.debug.enabled())) {
    return;
  }

  logger.set_context(msg.slot.sfn(), msg.slot.slot_index());
  logger.debug("Sector#{}: Slot.indication time_point={}", sector_id, msg.time_point.time_since_epoch().count());
}

void ocudu::fapi::log_ul_dci_request(const ul_dci_request& msg, unsigned sector_id, ocudulog::basic_logger& logger)
{
  if (OCUDU_LIKELY(!logger.debug.enabled())) {
    return;
  }

  fmt::memory_buffer buffer;
  fmt::format_to(std::back_inserter(buffer), "Sector#{}: UL_DCI.request slot={}", sector_id, msg.slot);

  for (const auto& pdu : msg.pdus) {
    log_pdcch_pdu(pdu.pdu, buffer);
  }

  logger.debug("{}", to_c_str(buffer));
}
