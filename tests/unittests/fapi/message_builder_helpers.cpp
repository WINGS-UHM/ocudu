/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "message_builder_helpers.h"
#include <random>

static std::mt19937 gen(0);

using namespace ocudu;
using namespace fapi;

static unsigned generate_sfn()
{
  std::uniform_int_distribution<unsigned> dist(0, 1023);
  return dist(gen);
}

static unsigned generate_slot()
{
  std::uniform_int_distribution<unsigned> dist(0, 159);

  return dist(gen);
}

static unsigned generate_start_symbol_index()
{
  std::uniform_int_distribution<unsigned> dist(0, 13);
  return dist(gen);
}

static ofdm_symbol_range generate_symbols(pucch_format type)
{
  std::uniform_int_distribution<unsigned> size_dist(0, 13);
  unsigned                                size = size_dist(gen);

  std::uniform_int_distribution<unsigned> start_dist(0, size);
  unsigned                                start = start_dist(gen);

  return {start, size};
}

static rnti_t generate_rnti()
{
  std::uniform_int_distribution<unsigned> dist(1, 65535);

  return to_rnti(dist(gen));
}

static unsigned generate_uint16()
{
  std::uniform_int_distribution<unsigned> dist(0, 65535);

  return dist(gen);
}

static harq_id_t generate_harq()
{
  std::uniform_int_distribution<unsigned> dist(0, 15);

  return static_cast<harq_id_t>(dist(gen));
}

static unsigned generate_handle()
{
  std::uniform_int_distribution<unsigned> dist(0, std::numeric_limits<uint32_t>::max());

  return dist(gen);
}

static unsigned generate_bwp_start()
{
  std::uniform_int_distribution<unsigned> dist(0, 274);

  return dist(gen);
}

static crb_interval generate_crb_interval()
{
  std::uniform_int_distribution<unsigned> size_dist(0, 274);
  unsigned                                size = size_dist(gen);

  std::uniform_int_distribution<unsigned> start_dist(0, size);
  unsigned                                start = start_dist(gen);

  return {start, size};
}

static prb_interval generate_prb_interval()
{
  std::uniform_int_distribution<unsigned> size_dist(0, 274);
  unsigned                                size = size_dist(gen);

  std::uniform_int_distribution<unsigned> start_dist(0, size);
  unsigned                                start = start_dist(gen);

  return {start, size};
}

static vrb_interval generate_vrb_interval()
{
  std::uniform_int_distribution<unsigned> size_dist(0, 274);
  unsigned                                size = size_dist(gen);

  std::uniform_int_distribution<unsigned> start_dist(0, size);
  unsigned                                start = start_dist(gen);

  return {start, size};
}

static subcarrier_spacing generate_scs()
{
  std::uniform_int_distribution<unsigned> dist(0, 4);

  return static_cast<subcarrier_spacing>(dist(gen));
}

static cyclic_prefix generate_cyclic_prefix()
{
  std::uniform_int_distribution<unsigned> dist(0, 1);

  return cyclic_prefix(static_cast<cyclic_prefix::options>(dist(gen)));
}

static pci_t generate_pci()
{
  std::uniform_int_distribution<unsigned> dist(0, 1007);

  return dist(gen);
}

static ssb_id_t generate_block_index()
{
  std::uniform_int_distribution<unsigned> dist(0, 63);

  return ssb_id_t(dist(gen));
}

static ssb_subcarrier_offset generate_subcarrier_offset()
{
  std::uniform_int_distribution<unsigned> dist(0, 23);

  return dist(gen);
}

static unsigned generate_offset_point_A()
{
  std::uniform_int_distribution<unsigned> dist(0, 2199);

  return dist(gen);
}

static ssb_pattern_case generate_case_pattern()
{
  std::uniform_int_distribution<unsigned> dist(0, 4);

  return static_cast<ssb_pattern_case>(dist(gen));
}

static bool generate_bool()
{
  std::uniform_int_distribution<unsigned> dist(0, 1);
  return dist(gen);
}

static coreset_id generate_coreset_id()
{
  std::uniform_int_distribution<unsigned> dist(0, 11);

  return static_cast<coreset_id>(dist(gen));
}

dl_ssb_pdu unittest::build_valid_dl_ssb_pdu()
{
  dl_ssb_pdu pdu;

  pdu.phys_cell_id        = generate_pci();
  pdu.beta_pss_profile_nr = beta_pss_profile_type::dB_0;
  pdu.ssb_block_index     = generate_block_index();
  pdu.subcarrier_offset   = generate_subcarrier_offset();
  pdu.ssb_offset_pointA   = generate_offset_point_A();
  pdu.bch_payload         = 0;
  pdu.case_type           = generate_case_pattern();
  pdu.scs                 = subcarrier_spacing::kHz240;
  pdu.L_max               = 4;

  return pdu;
}

dl_pdcch_pdu unittest::build_valid_dl_pdcch_pdu()
{
  dl_pdcch_pdu pdu;

  pdu.coreset_bwp                      = generate_crb_interval();
  pdu.scs                              = subcarrier_spacing::kHz240;
  pdu.cp                               = generate_cyclic_prefix();
  static constexpr pucch_format format = pucch_format::FORMAT_1;
  pdu.symbols                          = generate_symbols(format);
  uint8_t    reg_bundle_size           = 2;
  uint8_t    interleaver_size          = 3;
  uint16_t   shift_index               = 129;
  coreset_id id                        = generate_coreset_id();
  if (id == to_coreset_id(0)) {
    pdu.mapping = dl_pdcch_pdu::mapping_coreset_0{reg_bundle_size, interleaver_size, shift_index};
  } else {
    pdu.mapping = dl_pdcch_pdu::mapping_interleaved{reg_bundle_size, interleaver_size, shift_index};
  }
  pdu.precoder_granularity = static_cast<coreset_configuration::precoder_granularity_type>(generate_bool());

  pdu.freq_domain_resource.resize(pdu.freq_domain_resource.max_size());
  for (unsigned i = 0, e = pdu.freq_domain_resource.max_size(); i != e; ++i) {
    pdu.freq_domain_resource.set(i, generate_bool());
  }
  // Add the DCI.
  pdu.dl_dci.rnti                                                                               = generate_rnti();
  pdu.dl_dci.nid_pdcch_data                                                                     = generate_uint16();
  pdu.dl_dci.nrnti_pdcch_data                                                                   = generate_uint16();
  pdu.dl_dci.dci_aggregation_level                                                              = aggregation_level::n2;
  pdu.dl_dci.power_config.emplace<fapi::dl_dci_pdu::power_profile_nr>().power_control_offset_ss = 0;
  pdu.dl_dci.payload                   = {1, 1, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0};
  pdu.dl_dci.precoding_and_beamforming = build_valid_tx_precoding_and_beamforming_pdu();
  pdu.dl_dci.nid_pdcch_dmrs            = generate_uint16();

  return pdu;
}

dl_pdsch_pdu unittest::build_valid_dl_pdsch_pdu()
{
  dl_pdsch_pdu pdu;
  pdu.pdu_bitmap.set(1);
  pdu.rnti                                 = to_rnti(3);
  pdu.pdu_index                            = 2;
  pdu.bwp                                  = {3, 4};
  pdu.scs                                  = subcarrier_spacing::kHz15;
  pdu.cp                                   = cyclic_prefix::NORMAL;
  pdu.cws                                  = {{10, 2, 3, 1, 3, units::bytes{12}}};
  pdu.nid_pdsch                            = 65;
  pdu.num_layers                           = 6;
  pdu.transmission_scheme                  = 0;
  pdu.ref_point                            = pdsch_ref_point_type::point_a;
  pdu.pdsch_dmrs_scrambling_id             = 31;
  pdu.dmrs_type                            = dmrs_cfg_type::type_1;
  pdu.pdsch_dmrs_scrambling_id_compl       = 42;
  pdu.low_papr_dmrs                        = low_papr_dmrs_type::dependent_cdm_group;
  pdu.nscid                                = 0;
  pdu.num_dmrs_cdm_grps_no_data            = 2;
  pdu.resource_alloc                       = resource_allocation_type::type_1;
  pdu.vrbs                                 = {42, 89};
  pdu.vrb_to_prb_mapping                   = fapi::vrb_to_prb_mapping_type::interleaved_rb_size2;
  static constexpr pucch_format format     = pucch_format::FORMAT_1;
  pdu.symbols                              = generate_symbols(format);
  auto& power                              = pdu.power_config.emplace<dl_pdsch_pdu::power_profile_nr>();
  power.power_control_offset_profile_nr    = 6;
  power.power_control_offset_ss_profile_nr = fapi::power_control_offset_ss::dB3;
  pdu.is_inline_tb_crc                     = fapi::inline_tb_crc_type::control_message;
  pdu.dl_dmrs_symb_pos                     = 0;
  pdu.precoding_and_beamforming            = build_valid_tx_precoding_and_beamforming_pdu();

  // Maintenance v3.
  pdu.pdsch_maintenance_v3.trans_type = ocudu::fapi::pdsch_trans_type::interleaved_common_any_coreset0_not_present;
  pdu.pdsch_maintenance_v3.coreset_start_point                  = 2;
  pdu.pdsch_maintenance_v3.initial_dl_bwp_size                  = 3;
  pdu.pdsch_maintenance_v3.ldpc_base_graph                      = ocudu::ldpc_base_graph_type::BG1;
  pdu.pdsch_maintenance_v3.tb_size_lbrm_bytes                   = units::bytes{12};
  pdu.pdsch_maintenance_v3.prb_sym_rm_pattern_bitmap_size_byref = 0;
  pdu.pdsch_maintenance_v3.num_prb_sym_rm_patts_by_value        = 0;
  pdu.pdsch_maintenance_v3.num_coreset_rm_patterns              = 0;
  pdu.pdsch_maintenance_v3.pdcch_pdu_index                      = 4;
  pdu.pdsch_maintenance_v3.dci_index                            = 3;
  pdu.pdsch_maintenance_v3.max_num_cbg_per_tb                   = 4;

  // Maintenance v4
  pdu.pdsch_parameters_v4.lte_crs_mbsfn_derivation_method = 0;

  return pdu;
}

ocudu::fapi::dl_prs_pdu unittest::build_valid_dl_prs_pdu()
{
  dl_prs_pdu pdu;

  pdu.scs              = subcarrier_spacing::kHz240;
  pdu.cp               = cyclic_prefix::NORMAL;
  pdu.nid_prs          = 1;
  pdu.comb_size        = prs_comb_size::two;
  pdu.comb_offset      = 0;
  pdu.num_symbols      = prs_num_symbols::four;
  pdu.first_symbol     = 8;
  pdu.crbs             = {24, 28};
  pdu.prs_power_offset = -13.3;

  // Precoding.
  pdu.precoding_and_beamforming.prg_size = 276;
  // Use identity matrix
  pdu.precoding_and_beamforming.prg.pm_index = 0;

  return pdu;
}

dl_csi_rs_pdu unittest::build_valid_dl_csi_pdu()
{
  dl_csi_rs_pdu pdu;

  pdu.scs                                = subcarrier_spacing::kHz15;
  pdu.cp                                 = cyclic_prefix::NORMAL;
  pdu.crbs                               = {23, 28};
  pdu.type                               = csi_rs_type::CSI_RS_NZP;
  pdu.row                                = 1;
  pdu.freq_domain                        = {1, 0, 0, 0, 0, 0};
  pdu.symb_L0                            = 8;
  pdu.symb_L1                            = 7;
  pdu.cdm_type                           = csi_rs_cdm_type::no_CDM;
  pdu.freq_density                       = csi_rs_freq_density_type::three;
  pdu.scramb_id                          = 123;
  pdu.power_control_offset_profile_nr    = 0;
  pdu.power_control_offset_ss_profile_nr = power_control_offset_ss::dB0;
  pdu.bwp                                = {56U, 60U};

  return pdu;
}

dl_tti_request unittest::build_valid_dl_tti_request()
{
  dl_tti_request msg;
  msg.message_type = message_type_id::dl_tti_request;

  auto     scs        = subcarrier_spacing::kHz240;
  unsigned sfn        = 4U;
  auto     slot_index = 0U;
  msg.slot            = slot_point(scs, sfn, slot_index);

  // Manually add the SSB PDU to reuse the functions above.
  msg.pdus.emplace_back();
  msg.pdus.back().pdu_type = dl_pdu_type::SSB;
  msg.pdus.back().ssb_pdu  = build_valid_dl_ssb_pdu();

  // Manually add the PDCCH PDU to reuse the functions above.
  msg.pdus.emplace_back();
  msg.pdus.back().pdu_type  = dl_pdu_type::PDCCH;
  msg.pdus.back().pdcch_pdu = build_valid_dl_pdcch_pdu();

  // Manually add the PDSCH PDU to reuse the functions above.
  msg.pdus.emplace_back();
  msg.pdus.back().pdu_type  = dl_pdu_type::PDSCH;
  msg.pdus.back().pdsch_pdu = build_valid_dl_pdsch_pdu();

  // Manually add the CSI PDU to reuse the functions above.
  msg.pdus.emplace_back();
  msg.pdus.back().pdu_type   = dl_pdu_type::CSI_RS;
  msg.pdus.back().csi_rs_pdu = build_valid_dl_csi_pdu();

  return msg;
}

ul_dci_request unittest::build_valid_ul_dci_request()
{
  ul_dci_request msg;
  msg.message_type = message_type_id::ul_dci_request;

  auto     scs        = subcarrier_spacing::kHz240;
  unsigned sfn        = generate_sfn();
  auto     slot_index = generate_slot();
  msg.slot            = slot_point(scs, sfn, slot_index);

  // Manually add the PDCCH PDU to reuse the functions above.
  msg.pdus.emplace_back();
  msg.pdus.back().pdu = build_valid_dl_pdcch_pdu();

  return msg;
}

slot_indication unittest::build_valid_slot_indication()
{
  slot_indication msg;
  msg.message_type = message_type_id::slot_indication;

  auto     scs        = subcarrier_spacing::kHz240;
  unsigned sfn        = generate_sfn();
  auto     slot_index = generate_slot();
  msg.slot            = slot_point_extended(scs, 0, sfn, slot_index);

  return msg;
}

error_indication unittest::build_valid_error_indication()
{
  error_indication msg;
  msg.message_type             = message_type_id::error_indication;
  auto     scs                 = subcarrier_spacing::kHz240;
  unsigned sfn                 = generate_sfn();
  auto     slot_index          = generate_slot();
  msg.slot                     = slot_point(scs, sfn, slot_index);
  msg.message_id               = message_type_id::tx_data_request;
  msg.error_code               = error_code_id::msg_invalid_config;
  auto     expected_scs        = subcarrier_spacing::kHz240;
  unsigned expected_sfn        = generate_sfn();
  auto     expected_slot_index = generate_slot();
  msg.expected_slot            = slot_point(expected_scs, expected_sfn, expected_slot_index);

  return msg;
}

error_indication unittest::build_valid_out_of_sync_error_indication()
{
  error_indication msg;
  msg.message_type             = message_type_id::error_indication;
  auto     scs                 = subcarrier_spacing::kHz240;
  unsigned sfn                 = generate_sfn();
  auto     slot_index          = generate_slot();
  msg.slot                     = slot_point(scs, sfn, slot_index);
  msg.message_id               = message_type_id::tx_data_request;
  msg.error_code               = error_code_id::out_of_sync;
  auto     expected_scs        = subcarrier_spacing::kHz240;
  unsigned expected_sfn        = generate_sfn();
  auto     expected_slot_index = generate_slot();
  msg.expected_slot            = slot_point(expected_scs, expected_sfn, expected_slot_index);

  return msg;
}

error_indication unittest::build_valid_invalid_sfn_error_indication()
{
  error_indication msg;
  msg.message_type             = message_type_id::error_indication;
  auto     scs                 = subcarrier_spacing::kHz240;
  unsigned sfn                 = generate_sfn();
  auto     slot_index          = generate_slot();
  msg.slot                     = slot_point(scs, sfn, slot_index);
  msg.message_id               = message_type_id::ul_dci_request;
  msg.error_code               = error_code_id::msg_invalid_sfn;
  auto     expected_scs        = subcarrier_spacing::kHz240;
  unsigned expected_sfn        = std::numeric_limits<uint16_t>::max();
  auto     expected_slot_index = std::numeric_limits<uint16_t>::max();
  msg.expected_slot            = slot_point(expected_scs, expected_sfn, expected_slot_index);

  return msg;
}

error_indication unittest::build_valid_msg_error_indication()
{
  error_indication msg;
  msg.message_type    = message_type_id::error_indication;
  auto     scs        = subcarrier_spacing::kHz240;
  unsigned sfn        = generate_sfn();
  auto     slot_index = generate_slot();
  msg.slot            = slot_point(scs, sfn, slot_index);
  msg.message_id      = message_type_id::dl_tti_request;
  msg.error_code      = error_code_id::msg_slot_err;

  return msg;
}

error_indication unittest::build_valid_tx_err_error_indication()
{
  error_indication msg;
  msg.message_type             = message_type_id::error_indication;
  auto     scs                 = subcarrier_spacing::kHz240;
  unsigned sfn                 = generate_sfn();
  auto     slot_index          = generate_slot();
  msg.slot                     = slot_point(scs, sfn, slot_index);
  msg.message_id               = message_type_id::tx_data_request;
  msg.error_code               = error_code_id::msg_tx_err;
  auto     expected_scs        = subcarrier_spacing::kHz240;
  unsigned expected_sfn        = std::numeric_limits<uint16_t>::max();
  auto     expected_slot_index = std::numeric_limits<uint16_t>::max();
  msg.expected_slot            = slot_point(expected_scs, expected_sfn, expected_slot_index);

  return msg;
}

error_indication unittest::build_valid_ul_dci_err_error_indication()
{
  error_indication msg;
  msg.message_type    = message_type_id::error_indication;
  auto     scs        = subcarrier_spacing::kHz240;
  unsigned sfn        = generate_sfn();
  auto     slot_index = generate_slot();
  msg.slot            = slot_point(scs, sfn, slot_index);
  msg.message_id      = message_type_id::ul_dci_request;
  msg.error_code      = error_code_id::msg_ul_dci_err;

  return msg;
}

rx_data_indication unittest::build_valid_rx_data_indication()
{
  rx_data_indication msg;
  msg.message_type = message_type_id::rx_data_indication;

  auto     scs        = subcarrier_spacing::kHz240;
  unsigned sfn        = generate_sfn();
  auto     slot_index = generate_slot();
  msg.slot            = slot_point(scs, sfn, slot_index);

  msg.pdus.emplace_back();
  auto& pdu                                            = msg.pdus.back();
  pdu.handle                                           = generate_handle();
  pdu.rnti                                             = generate_rnti();
  pdu.harq_id                                          = generate_harq();
  static std::array<uint8_t, 10> fixed_transport_block = {};
  pdu.transport_block                                  = fixed_transport_block;

  return msg;
}

static uint8_t generate_index_fd_ra()
{
  std::uniform_int_distribution<unsigned> dist(0, 7);

  return dist(gen);
}

static uint8_t generate_prach_start_symbol()
{
  std::uniform_int_distribution<unsigned> dist(0, 13);

  return dist(gen);
}

static uint8_t generate_num_cs()
{
  std::uniform_int_distribution<unsigned> dist(0, 419);

  return dist(gen);
}

static prach_format_type generate_prach_format()
{
  std::uniform_int_distribution<unsigned> dist(0, 3);

  return static_cast<prach_format_type>(dist(gen));
}

static ul_prach_pdu::preambles_interval generate_preambles()
{
  std::uniform_int_distribution<unsigned> num_preambles_dist(1, 64);

  unsigned num_preambles = num_preambles_dist(gen);

  unsigned start_preamble = std::min(static_cast<unsigned>(generate_num_cs()), num_preambles);

  return {num_preambles, start_preamble};
}

ul_prach_pdu unittest::build_valid_ul_prach_pdu()
{
  ul_prach_pdu pdu;

  pdu.num_prach_ocas     = 1;
  pdu.prach_format       = generate_prach_format();
  pdu.index_fd_ra        = generate_index_fd_ra();
  pdu.prach_start_symbol = generate_prach_start_symbol();
  pdu.num_cs             = generate_num_cs();
  pdu.num_fd_ra          = generate_index_fd_ra() + 1U;
  pdu.preambles          = generate_preambles();

  return pdu;
}

static int generate_ul_sinr_metric()
{
  std::uniform_int_distribution<int> dist(-32768, 32767);
  return dist(gen);
}

static phy_time_unit generate_timing_advance_offset()
{
  std::uniform_int_distribution<unsigned> dist(0, 100);
  unsigned                                value = dist(gen);

  return phy_time_unit::from_seconds((value < 64U) ? value : 65535U);
}

static unsigned generate_rssi_or_rsrp()
{
  std::uniform_int_distribution<unsigned> dist(0, 2500);
  unsigned                                value = dist(gen);

  return (value <= 1280U) ? value : 65535;
}

static units::bits generate_bit_length()
{
  std::uniform_int_distribution<unsigned> dist(1, 1706);
  return units::bits(dist(gen));
}

static uci_harq_pdu generate_harq_pdu()
{
  uci_harq_pdu pdu;

  pdu.detection_status    = uci_pusch_or_pucch_f2_3_4_detection_status::crc_pass;
  pdu.expected_bit_length = generate_bit_length();
  pdu.payload.resize(pdu.expected_bit_length.value());

  return pdu;
}

static uci_csi_part1 generate_csi_part1_pdu()
{
  uci_csi_part1 pdu;

  pdu.detection_status    = uci_pusch_or_pucch_f2_3_4_detection_status::crc_pass;
  pdu.expected_bit_length = generate_bit_length();
  pdu.payload.resize(pdu.expected_bit_length.value());

  return pdu;
}

static uci_csi_part2 generate_csi_part2_pdu()
{
  uci_csi_part2 pdu;

  pdu.detection_status    = uci_pusch_or_pucch_f2_3_4_detection_status::crc_pass;
  pdu.expected_bit_length = generate_bit_length();
  pdu.payload.resize(pdu.expected_bit_length.value());

  return pdu;
}

uci_pusch_pdu unittest::build_valid_uci_pusch_pdu()
{
  uci_pusch_pdu pdu;

  pdu.handle                = generate_handle();
  pdu.rnti                  = generate_rnti();
  pdu.ul_sinr_metric        = static_cast<int16_t>(generate_ul_sinr_metric());
  pdu.timing_advance_offset = generate_timing_advance_offset();
  pdu.rssi                  = generate_rssi_or_rsrp();
  pdu.rsrp                  = generate_rssi_or_rsrp();

  // Enable HARQ, CSI Part 1 and CSI Part 2.
  pdu.harq      = generate_harq_pdu();
  pdu.csi_part1 = generate_csi_part1_pdu();
  pdu.csi_part2 = generate_csi_part2_pdu();

  return pdu;
}

static sr_pdu_format_0_1 generate_sr_format01_pdu()
{
  sr_pdu_format_0_1 pdu;

  std::uniform_int_distribution<unsigned> dist(0, 3);
  pdu.sr_detected = std::min(1U, dist(gen));

  return pdu;
}

static uci_harq_format_0_1 generate_harq_format01_pdu()
{
  uci_harq_format_0_1 pdu;

  std::uniform_int_distribution<unsigned> dist(0, 2);

  std::uniform_int_distribution<unsigned> nof_dist(1, 2);
  for (unsigned i = 0, e = nof_dist(gen); i != e; ++i) {
    pdu.harq_values.emplace_back(static_cast<uci_pucch_f0_or_f1_harq_values>(dist(gen)));
  }

  return pdu;
}

uci_pucch_pdu_format_0_1 unittest::build_valid_uci_pucch_format01_pdu()
{
  uci_pucch_pdu_format_0_1 pdu;

  pdu.handle                = generate_handle();
  pdu.rnti                  = generate_rnti();
  pdu.ul_sinr_metric        = static_cast<int16_t>(generate_ul_sinr_metric());
  pdu.timing_advance_offset = generate_timing_advance_offset();
  pdu.rssi                  = generate_rssi_or_rsrp();
  pdu.rsrp                  = generate_rssi_or_rsrp();

  std::uniform_int_distribution<unsigned> dist(0, 1);
  pdu.pucch_format = static_cast<uci_pucch_pdu_format_0_1::format_type>(dist(gen));

  // Enable SR and HARQ for PUCCH format 0/1.
  pdu.sr   = generate_sr_format01_pdu();
  pdu.harq = generate_harq_format01_pdu();

  return pdu;
}

static sr_pdu_format_2_3_4 generate_sr_format234_pdu()
{
  sr_pdu_format_2_3_4 pdu;

  std::uniform_int_distribution<unsigned> dist(1, 4);
  pdu.sr_payload.resize(std::ceil(static_cast<float>(dist(gen)) / 8.F));

  return pdu;
}

uci_pucch_pdu_format_2_3_4 unittest::build_valid_uci_pucch_format234_pdu()
{
  uci_pucch_pdu_format_2_3_4 pdu;

  pdu.handle                = generate_handle();
  pdu.rnti                  = generate_rnti();
  pdu.ul_sinr_metric        = static_cast<int16_t>(generate_ul_sinr_metric());
  pdu.timing_advance_offset = generate_timing_advance_offset();
  pdu.rssi                  = generate_rssi_or_rsrp();
  pdu.rsrp                  = generate_rssi_or_rsrp();

  std::uniform_int_distribution<unsigned> dist(0, 2);
  pdu.pucch_format = static_cast<uci_pucch_pdu_format_2_3_4::format_type>(dist(gen));

  // Enable SR, HARQ, CSI Part 1. CSI Part 2, UCI payload Part 1 and UCI payload Part 2.
  pdu.sr        = generate_sr_format234_pdu();
  pdu.harq      = generate_harq_pdu();
  pdu.csi_part1 = generate_csi_part1_pdu();
  pdu.csi_part2 = generate_csi_part2_pdu();

  return pdu;
}

uci_indication unittest::build_valid_uci_indication()
{
  std::uniform_int_distribution<unsigned> sfn_dist(0, 1023);
  std::uniform_int_distribution<unsigned> slot_dist(0, 159);

  uci_indication msg;
  msg.message_type = message_type_id::uci_indication;

  auto     scs        = subcarrier_spacing::kHz240;
  unsigned sfn        = sfn_dist(gen);
  auto     slot_index = slot_dist(gen);
  msg.slot            = slot_point(scs, sfn, slot_index);

  // Add one PDU of each type.
  msg.pdus.emplace_back();
  msg.pdus.back() = build_valid_uci_pusch_pdu();

  // Add one PDU of each type.
  msg.pdus.emplace_back();
  msg.pdus.back() = build_valid_uci_pucch_format01_pdu();

  // Add one PDU of each type.
  msg.pdus.emplace_back();
  msg.pdus.back() = build_valid_uci_pucch_format234_pdu();

  return msg;
}

static pucch_repetition_tx_slot generate_multi_slot_tx_indicator()
{
  std::uniform_int_distribution<unsigned> dist(0, 3);
  return static_cast<pucch_repetition_tx_slot>(dist(gen));
}

static unsigned generate_sr_bit_len(pucch_format type)
{
  if (type == pucch_format::FORMAT_0 || type == pucch_format::FORMAT_1) {
    std::uniform_int_distribution<unsigned> dist(0, 1);
    return dist(gen);
  }

  std::uniform_int_distribution<unsigned> dist(0, 4);
  return dist(gen);
}

static unsigned generate_harq_bit_len(pucch_format type)
{
  if (type == pucch_format::FORMAT_0 || type == pucch_format::FORMAT_1) {
    std::uniform_int_distribution<unsigned> dist(1, 2);
    return dist(gen);
  }

  std::uniform_int_distribution<unsigned> dist(2, 1706);
  return dist(gen);
}

static unsigned generate_max_code_rate(pucch_format type)
{
  if (type == pucch_format::FORMAT_0 || type == pucch_format::FORMAT_1) {
    return 255;
  }

  std::uniform_int_distribution<unsigned> dist(0, 7);
  return dist(gen);
}

static pucch_group_hopping generate_pucch_group_hopping()
{
  std::uniform_int_distribution<unsigned> dist(0, 2);
  return static_cast<pucch_group_hopping>(dist(gen));
}

static unsigned generate_nid_pucch_hopping()
{
  std::uniform_int_distribution<unsigned> dist(0, 1023);
  return dist(gen);
}

static unsigned generate_initial_cyclic_shift()
{
  std::uniform_int_distribution<unsigned> dist(0, 11);
  return dist(gen);
}

static unsigned generate_csi_bit_len()
{
  std::uniform_int_distribution<unsigned> dist(0, 1706);
  return dist(gen);
}

static ul_pucch_pdu generate_generic_ul_pucch_pdu()
{
  ul_pucch_pdu pdu;

  pdu.rnti                           = generate_rnti();
  pdu.handle                         = generate_handle();
  pdu.bwp                            = generate_crb_interval();
  pdu.scs                            = generate_scs();
  pdu.cp                             = generate_cyclic_prefix();
  pdu.multi_slot_tx_indicator        = generate_multi_slot_tx_indicator();
  pdu.pi2_bpsk                       = true;
  pdu.prbs                           = generate_prb_interval();
  pdu.intra_slot_frequency_hopping   = true;
  pdu.second_hop_prb                 = generate_bwp_start();
  pdu.pucch_grp_hopping              = generate_pucch_group_hopping();
  pdu.nid_pucch_hopping              = generate_nid_pucch_hopping();
  pdu.initial_cyclic_shift           = generate_initial_cyclic_shift();
  pdu.nid_pucch_scrambling           = generate_nid_pucch_hopping();
  pdu.time_domain_occ_index          = 5;
  pdu.pre_dft_occ_len                = 2;
  pdu.pre_dft_occ_idx                = 2;
  pdu.add_dmrs_flag                  = true;
  pdu.nid0_pucch_dmrs_scrambling     = 10200;
  pdu.m0_pucch_dmrs_cyclic_shift     = 8;
  pdu.csi_part1_bit_length           = generate_csi_bit_len();
  pdu.pucch_maintenance_v3.ul_bwp_id = 43;

  // Add 1 part1 to part2 correspondence.
  pdu.uci_correspondence.part2.emplace_back();
  auto& corr                = pdu.uci_correspondence.part2.back();
  corr.priority             = 3;
  corr.param_offsets        = {1, 2, 3};
  corr.param_sizes          = {1, 2, 3};
  corr.part2_size_map_index = 43;
  corr.part2_size_map_scope = uci_part1_to_part2_correspondence_v3::map_scope_type::common_context;

  return pdu;
}

ul_pucch_pdu unittest::build_valid_ul_pucch_f0_pdu()
{
  auto                          pdu      = generate_generic_ul_pucch_pdu();
  static constexpr pucch_format format   = pucch_format::FORMAT_0;
  pdu.format_type                        = format;
  pdu.symbols                            = generate_symbols(format);
  pdu.sr_bit_len                         = generate_sr_bit_len(format);
  pdu.bit_len_harq                       = generate_harq_bit_len(format);
  pdu.pucch_maintenance_v3.max_code_rate = generate_max_code_rate(format);

  return pdu;
}

ul_pucch_pdu unittest::build_valid_ul_pucch_f1_pdu()
{
  auto                          pdu      = generate_generic_ul_pucch_pdu();
  static constexpr pucch_format format   = pucch_format::FORMAT_1;
  pdu.format_type                        = format;
  pdu.symbols                            = generate_symbols(format);
  pdu.sr_bit_len                         = generate_sr_bit_len(format);
  pdu.bit_len_harq                       = generate_harq_bit_len(format);
  pdu.pucch_maintenance_v3.max_code_rate = generate_max_code_rate(format);

  return pdu;
}

ul_pucch_pdu unittest::build_valid_ul_pucch_f2_pdu()
{
  auto                          pdu      = generate_generic_ul_pucch_pdu();
  static constexpr pucch_format format   = pucch_format::FORMAT_2;
  pdu.format_type                        = format;
  pdu.symbols                            = generate_symbols(format);
  pdu.sr_bit_len                         = generate_sr_bit_len(format);
  pdu.bit_len_harq                       = generate_harq_bit_len(format);
  pdu.pucch_maintenance_v3.max_code_rate = generate_max_code_rate(format);

  return pdu;
}

ul_pucch_pdu unittest::build_valid_ul_pucch_f3_pdu()
{
  auto                          pdu      = generate_generic_ul_pucch_pdu();
  static constexpr pucch_format format   = pucch_format::FORMAT_3;
  pdu.format_type                        = format;
  pdu.symbols                            = generate_symbols(format);
  pdu.sr_bit_len                         = generate_sr_bit_len(format);
  pdu.bit_len_harq                       = generate_harq_bit_len(format);
  pdu.pucch_maintenance_v3.max_code_rate = generate_max_code_rate(format);

  return pdu;
}

ul_pucch_pdu unittest::build_valid_ul_pucch_f4_pdu()
{
  auto                          pdu      = generate_generic_ul_pucch_pdu();
  static constexpr pucch_format format   = pucch_format::FORMAT_4;
  pdu.format_type                        = format;
  pdu.symbols                            = generate_symbols(format);
  pdu.sr_bit_len                         = generate_sr_bit_len(format);
  pdu.bit_len_harq                       = generate_harq_bit_len(format);
  pdu.pucch_maintenance_v3.max_code_rate = generate_max_code_rate(format);

  return pdu;
}

static modulation_scheme generate_qam_mod_order(bool transform_precoding)
{
  static constexpr std::array<unsigned, 5> values = {1, 2, 4, 6, 8};

  std::uniform_int_distribution<unsigned> dist(0, 4);
  unsigned                                index = dist(gen);

  if (!transform_precoding && index == 0) {
    ++index;
  }

  return static_cast<modulation_scheme>(values[index]);
}

static unsigned generate_mcs_index()
{
  std::uniform_int_distribution<unsigned> dist(0, 31);
  return dist(gen);
}

static pusch_mcs_table generate_mcs_table()
{
  std::uniform_int_distribution<unsigned> dist(0, 4);
  return static_cast<pusch_mcs_table>(dist(gen));
}

static unsigned generate_num_layers()
{
  std::uniform_int_distribution<unsigned> dist(1, 4);
  return dist(gen);
}

static dmrs_cfg_type generate_dmrs_type()
{
  std::uniform_int_distribution<unsigned> dist(0, 1);
  return static_cast<dmrs_cfg_type>(dist(gen));
}

static low_papr_dmrs_type generate_low_papr_dmrs()
{
  std::uniform_int_distribution<unsigned> dist(0, 1);
  return static_cast<low_papr_dmrs_type>(dist(gen));
}

static unsigned generate_pusch_dmrs_identity()
{
  std::uniform_int_distribution<unsigned> dist(0, 1007);
  return dist(gen);
}

static unsigned generate_num_dmrs_cdm_no_data()
{
  std::uniform_int_distribution<unsigned> dist(1, 3);
  return dist(gen);
}

static unsigned generate_tx_direct_current_location()
{
  std::uniform_int_distribution<unsigned> dist(0, 4095);
  return dist(gen);
}

static ldpc_base_graph_type generate_ldpc_graph_type()
{
  std::uniform_int_distribution<unsigned> dist(1, 2);
  return static_cast<ldpc_base_graph_type>(dist(gen));
}

ul_pusch_pdu unittest::build_valid_ul_pusch_pdu()
{
  ul_pusch_pdu pdu;

  pdu.rb_bitmap                           = {};
  pdu.rnti                                = generate_rnti();
  pdu.handle                              = generate_handle();
  pdu.bwp                                 = generate_crb_interval();
  pdu.scs                                 = generate_scs();
  pdu.cp                                  = generate_cyclic_prefix();
  pdu.target_code_rate                    = 1982U;
  pdu.transform_precoding                 = generate_bool();
  pdu.qam_mod_order                       = generate_qam_mod_order(pdu.transform_precoding);
  pdu.mcs_index                           = generate_mcs_index();
  pdu.mcs_table                           = generate_mcs_table();
  pdu.nid_pusch                           = generate_nid_pucch_hopping();
  pdu.num_layers                          = generate_num_layers();
  pdu.ul_dmrs_symb_pos                    = 3U;
  pdu.dmrs_type                           = generate_dmrs_type();
  pdu.pusch_dmrs_scrambling_id            = 32421;
  pdu.pusch_dmrs_scrambling_id_complement = 3213;
  pdu.low_papr_dmrs                       = generate_low_papr_dmrs();
  pdu.pusch_dmrs_identity                 = generate_pusch_dmrs_identity();
  pdu.nscid                               = generate_bool();
  pdu.num_dmrs_cdm_grps_no_data           = generate_num_dmrs_cdm_no_data();
  pdu.dmrs_ports                          = 4;
  pdu.resource_alloc                      = resource_allocation_type::type_1;
  pdu.vrbs                                = generate_vrb_interval();
  pdu.vrb_to_prb_mapping                  = vrb_to_prb_mapping_type::non_interleaved;
  pdu.intra_slot_frequency_hopping        = generate_bool();
  pdu.tx_direct_current_location          = generate_tx_direct_current_location();
  pdu.uplink_frequency_shift_7p5kHz       = generate_bool();
  static constexpr pucch_format format    = pucch_format::FORMAT_1;
  pdu.symbols                             = generate_symbols(format);

  auto& data = pdu.pusch_data;
  pdu.pdu_bitmap.set(ul_pusch_pdu::PUSCH_DATA_BIT);
  data.rv_index        = 2;
  data.harq_process_id = 2;
  data.new_data        = false;
  data.tb_size         = units::bytes{213131};
  data.num_cb          = 3414;

  auto& uci = pdu.pusch_uci;
  pdu.pdu_bitmap.set(ul_pusch_pdu::PUSCH_UCI_BIT);
  uci.harq_ack_bit_length  = 3;
  uci.csi_part1_bit_length = 4;
  uci.flags_csi_part2      = 65535;
  uci.alpha_scaling        = alpha_scaling_opt::f0p5;
  uci.beta_offset_harq_ack = 12;
  uci.beta_offset_csi1     = 16;
  uci.beta_offset_csi2     = 17;

  // Add 1 part1 to part2 correspondence.
  pdu.uci_correspondence.part2.emplace_back();
  auto& corr                = pdu.uci_correspondence.part2.back();
  corr.priority             = 3;
  corr.param_offsets        = {1, 2};
  corr.param_sizes          = {1, 2};
  corr.part2_size_map_index = 0;
  corr.part2_size_map_scope = uci_part1_to_part2_correspondence_v3::map_scope_type::common_context;

  auto& ptrs = pdu.pusch_ptrs;
  pdu.pdu_bitmap.set(ul_pusch_pdu::PUSCH_PTRS_BIT);
  ptrs.ul_ptrs_power     = ul_ptrs_power_type::dB4_77;
  ptrs.ptrs_freq_density = 1;
  ptrs.ptrs_time_density = 2;
  ptrs.port_info.push_back({3, 4, 5});

  auto& ofdm = pdu.pusch_ofdm;
  pdu.pdu_bitmap.set(ul_pusch_pdu::DFTS_OFDM_BIT);
  ofdm.low_papr_group_number                    = 25;
  ofdm.low_papr_sequence_number                 = 3232;
  ofdm.ul_ptrs_sample_density                   = 3;
  ofdm.ul_ptrs_time_density_transform_precoding = 4;

  auto& v3                            = pdu.pusch_maintenance_v3;
  v3.pusch_trans_type                 = 2;
  v3.delta_bwp0_start_from_active_bwp = generate_bwp_start();
  v3.initial_ul_bwp_size              = generate_bwp_start();
  v3.group_or_sequence_hopping        = 2;
  v3.pusch_second_hop_prb             = generate_bwp_start();
  v3.ldpc_base_graph                  = generate_ldpc_graph_type();
  v3.tb_size_lbrm_bytes               = units::bytes{32323242};

  pdu.pusch_params_v4.cb_crc_status_request = generate_bool();

  return pdu;
}

ul_srs_pdu unittest::build_valid_ul_srs_pdu()
{
  ul_srs_pdu pdu;
  pdu.rnti                               = to_rnti(23);
  pdu.handle                             = 8;
  pdu.bwp                                = {230, 10};
  pdu.scs                                = subcarrier_spacing::kHz240;
  pdu.cp                                 = cyclic_prefix::NORMAL;
  pdu.num_ant_ports                      = srs_resource_configuration::one_two_four_enum::two;
  pdu.ofdm_symbols                       = ofdm_symbol_range();
  pdu.num_repetitions                    = n1;
  pdu.time_start_position                = 3;
  pdu.config_index                       = 4;
  pdu.sequence_id                        = 6;
  pdu.bandwidth_index                    = 1;
  pdu.comb_size                          = tx_comb_size::n2;
  pdu.comb_offset                        = 1;
  pdu.cyclic_shift                       = 0;
  pdu.frequency_position                 = 3;
  pdu.frequency_shift                    = 10;
  pdu.frequency_hopping                  = 2;
  pdu.group_or_sequence_hopping          = srs_group_or_sequence_hopping::neither;
  pdu.resource_type                      = srs_resource_type::periodic;
  pdu.t_srs                              = srs_periodicity::sl4;
  pdu.t_offset                           = 2;
  pdu.enable_normalized_iq_matrix_report = false;
  pdu.enable_positioning_report          = false;

  return pdu;
}

ul_tti_request unittest::build_valid_ul_tti_request()
{
  ul_tti_request msg;
  msg.message_type = message_type_id::ul_tti_request;

  auto     scs        = subcarrier_spacing::kHz240;
  unsigned sfn        = generate_sfn();
  auto     slot_index = generate_slot();
  msg.slot            = slot_point(scs, sfn, slot_index);
  msg.num_groups      = 2000;
  msg.num_pdus_of_each_type.fill(1);

  {
    ++msg.num_pdus_of_each_type[static_cast<unsigned>(ul_tti_request::pdu_type::PRACH)];
    msg.pdus.push_back({ul_pdu_type::PRACH, 0, build_valid_ul_prach_pdu()});
  }
  {
    ++msg.num_pdus_of_each_type[static_cast<unsigned>(ul_tti_request::pdu_type::PUCCH_format01)];
    ul_tti_request_pdu pdu;
    pdu.pdu_type  = ul_pdu_type::PUCCH;
    pdu.pucch_pdu = build_valid_ul_pucch_f0_pdu();
    msg.pdus.push_back(pdu);
  }
  {
    ++msg.num_pdus_of_each_type[static_cast<unsigned>(ul_tti_request::pdu_type::PUCCH_format234)];
    ul_tti_request_pdu pdu;
    pdu.pdu_type  = ul_pdu_type::PUCCH;
    pdu.pucch_pdu = build_valid_ul_pucch_f3_pdu();
    msg.pdus.push_back(pdu);
  }
  {
    ++msg.num_pdus_of_each_type[static_cast<unsigned>(ul_tti_request::pdu_type::PUSCH)];
    ul_tti_request_pdu pdu;
    pdu.pdu_type  = ul_pdu_type::PUSCH;
    pdu.pusch_pdu = build_valid_ul_pusch_pdu();
    msg.pdus.push_back(pdu);
  }

  return msg;
}

static unsigned generate_slot_index()
{
  std::uniform_int_distribution<unsigned> dist(0, 79);
  return dist(gen);
}

static unsigned generate_ra_index()
{
  std::uniform_int_distribution<unsigned> dist(0, 7);
  return dist(gen);
}

static unsigned generate_preamble_index()
{
  std::uniform_int_distribution<unsigned> dist(0, 63);
  return dist(gen);
}

rach_indication unittest::build_valid_rach_indication()
{
  rach_indication msg;
  msg.message_type = message_type_id::rach_indication;

  auto     scs        = subcarrier_spacing::kHz240;
  unsigned sfn        = generate_sfn();
  auto     slot_index = generate_slot();
  msg.slot            = slot_point(scs, sfn, slot_index);

  msg.pdus.emplace_back();
  rach_indication_pdu& pdu = msg.pdus.back();

  pdu.symbol_index = generate_start_symbol_index();
  pdu.slot_index   = generate_slot_index();
  pdu.ra_index     = generate_ra_index();
  pdu.avg_rssi     = std::numeric_limits<uint32_t>::max();
  pdu.avg_snr      = std::numeric_limits<uint8_t>::max();

  pdu.preambles.emplace_back();
  rach_indication_pdu_preamble& preamble = pdu.preambles.back();

  preamble.preamble_index        = generate_preamble_index();
  preamble.timing_advance_offset = phy_time_unit();
  preamble.preamble_pwr          = std::numeric_limits<uint32_t>::max();
  preamble.preamble_snr          = std::numeric_limits<uint8_t>::max();

  return msg;
}

tx_data_request unittest::build_valid_tx_data_request()
{
  tx_data_request msg;
  msg.message_type = message_type_id::tx_data_request;

  auto     scs        = subcarrier_spacing::kHz240;
  unsigned sfn        = generate_sfn();
  auto     slot_index = generate_slot();
  msg.slot            = slot_point(scs, sfn, slot_index);

  msg.pdus.emplace_back();
  tx_data_req_pdu& pdu = msg.pdus.back();

  pdu.cw_index  = generate_bool();
  pdu.pdu_index = 4231;

  static std::array<uint8_t, 4> data;
  pdu.pdu = shared_transport_block(data);

  return msg;
}

ocudu::fapi::crc_indication unittest::build_valid_crc_indication()
{
  crc_indication msg;
  msg.message_type = message_type_id::crc_indication;

  msg.slot = slot_point(subcarrier_spacing::kHz240, 238, 3);
  msg.pdus.emplace_back();
  crc_ind_pdu& pdu          = msg.pdus.front();
  pdu.rnti                  = to_rnti(34);
  pdu.harq_id               = to_harq_id(2);
  pdu.tb_crc_status_ok      = true;
  pdu.ul_sinr_metric        = 0;
  pdu.timing_advance_offset = phy_time_unit();
  pdu.rssi                  = 100;
  pdu.rsrp                  = 10;

  return msg;
}

ocudu::fapi::tx_precoding_and_beamforming_pdu unittest::build_valid_tx_precoding_and_beamforming_pdu()
{
  ocudu::fapi::tx_precoding_and_beamforming_pdu pdu;

  pdu.prg_size = 1U;
  pdu.prg      = {3U};

  return pdu;
}

srs_indication unittest::build_valid_srs_indication()
{
  srs_indication msg;
  msg.message_type = message_type_id::srs_indication;

  auto     scs        = subcarrier_spacing::kHz240;
  unsigned sfn        = generate_sfn();
  auto     slot_index = generate_slot();
  msg.slot            = slot_point(scs, sfn, slot_index);

  msg.pdus.emplace_back();
  auto& pdu                 = msg.pdus.back();
  pdu.handle                = generate_handle();
  pdu.rnti                  = generate_rnti();
  pdu.timing_advance_offset = generate_timing_advance_offset();

  return msg;
}
