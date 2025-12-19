/*
 *
 * Copyright 2021-2025 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "lib/scheduler/scheduler_impl.h"
#include "test_utils/dummy_test_components.h"
#include "tests/test_doubles/scheduler/cell_config_builder_profiles.h"
#include "tests/test_doubles/scheduler/pucch_res_test_builder_helper.h"
#include "tests/test_doubles/scheduler/scheduler_config_helper.h"
#include "tests/unittests/scheduler/test_utils/config_generators.h"
#include "tests/unittests/scheduler/test_utils/scheduler_test_suite.h"
#include "ocudu/ran/duplex_mode.h"
#include "ocudu/ran/pdcch/search_space.h"
#include "ocudu/ran/pusch/pusch_configuration.h"
#include "ocudu/scheduler/config/logical_channel_config_factory.h"
#include "ocudu/scheduler/result/dci_info.h"
#include "ocudu/support/test_utils.h"
#include <gtest/gtest.h>
#include <unordered_map>

using namespace ocudu;

struct sched_test_ue {
  rnti_t                                                         crnti;
  std::unordered_map<lcg_id_t, ul_bsr_lcg_report>                ul_bsr_list;
  std::unordered_map<lcid_t, dl_buffer_state_indication_message> dl_bsr_list;
  sched_ue_creation_request_message                              msg;
};

unsigned allocate_rnti()
{
  // Randomly chosen RNTI start.
  static unsigned rnti_start = 0x4601;
  return rnti_start++;
}

// Helper class to initialize and store relevant objects for the test and provide helper methods.
struct test_bench {
  // Maximum number of slots to run per UE in order to validate the results of scheduler. Implementation defined.
  static constexpr unsigned max_test_run_slots_per_ue = 80;

  scheduler_expert_config                          expert_cfg;
  cell_configuration                               cell_cfg;
  sched_cfg_dummy_notifier                         cfg_notif;
  scheduler_ue_metrics_dummy_notifier              metric_notif;
  scheduler_impl                                   sch;
  std::unordered_map<du_ue_index_t, sched_test_ue> ues;
  const sched_result*                              sched_res{nullptr};

  explicit test_bench(const scheduler_expert_config&                  expert_cfg_,
                      const sched_cell_configuration_request_message& cell_req) :
    expert_cfg{expert_cfg_}, cell_cfg{expert_cfg, cell_req}, sch{scheduler_config{expert_cfg, cfg_notif}}
  {
    sch.handle_cell_configuration_request(cell_req);
  }

  du_ue_index_t rnti_to_du_ue_index(rnti_t rnti)
  {
    for (const auto& u : ues) {
      if (u.second.crnti == rnti) {
        return u.first;
      }
    }
    return du_ue_index_t::INVALID_DU_UE_INDEX;
  }
};

class scheduler_impl_tester
{
protected:
  slot_point                    current_slot{0, 0};
  ocudulog::basic_logger&       mac_logger  = ocudulog::fetch_basic_logger("SCHED", true);
  ocudulog::basic_logger&       test_logger = ocudulog::fetch_basic_logger("TEST", true);
  std::optional<test_bench>     bench;
  pucch_res_builder_test_helper pucch_cfg_builder;

  // We use this value to account for the case when the PDSCH or PUSCH is allocated several slots in advance.
  unsigned max_k_value = 0;

  ~scheduler_impl_tester()
  {
    // Log pending allocations before finishing test.
    for (unsigned i = 0; i != max_k_value; ++i) {
      run_slot();
    }
    ocudulog::flush();
  }

  void setup_sched(const scheduler_expert_config& expert_cfg, sched_cell_configuration_request_message msg)
  {
    current_slot = slot_point{to_numerology_value(msg.scs_common), 0};

    const auto& dl_lst = msg.dl_cfg_common.init_dl_bwp.pdsch_common.pdsch_td_alloc_list;
    for (const auto& pdsch : dl_lst) {
      if (pdsch.k0 > max_k_value) {
        max_k_value = pdsch.k0;
      }
    }
    const auto& ul_lst = msg.ul_cfg_common.init_ul_bwp.pusch_cfg_common->pusch_td_alloc_list;
    for (const auto& pusch : ul_lst) {
      if (pusch.k2 > max_k_value) {
        max_k_value = pusch.k2;
      }
    }

    pucch_builder_params pucch_basic_params{
        .res_set_0_size = 8, .res_set_1_size = 8, .nof_cell_sr_resources = 8, .nof_cell_csi_resources = 8};
    auto& f1_params          = pucch_basic_params.f0_or_f1_params.emplace<pucch_f1_params>();
    f1_params.nof_cyc_shifts = pucch_nof_cyclic_shifts::twelve;
    f1_params.occ_supported  = true;

    msg.ded_pucch_resources = config_helpers::build_pucch_resource_list(
        pucch_basic_params, msg.ul_cfg_common.init_ul_bwp.generic_params.crbs.length());
    bench.emplace(expert_cfg, msg);

    // Initialize.
    mac_logger.set_context(current_slot.sfn(), current_slot.slot_index());
    test_logger.set_context(current_slot.sfn(), current_slot.slot_index());
    bench->sched_res = &bench->sch.slot_indication(current_slot, to_du_cell_index(0));

    pucch_cfg_builder.setup(bench->cell_cfg, pucch_basic_params);
  }

  void run_slot()
  {
    ++current_slot;

    mac_logger.set_context(current_slot.sfn(), current_slot.slot_index());
    test_logger.set_context(current_slot.sfn(), current_slot.slot_index());

    bench->sched_res = &bench->sch.slot_indication(current_slot, to_du_cell_index(0));

    // Check sched result consistency.
    test_scheduler_result_consistency(bench->cell_cfg, current_slot, *bench->sched_res);
  }

  static scheduler_expert_config create_expert_config(sch_mcs_index            max_msg4_mcs_index,
                                                      vrb_to_prb::mapping_type pdsch_interleaving_bundle_size,
                                                      bool                     enable_csi_rs_pdsch_multiplexing = true)
  {
    auto cfg                                = config_helpers::make_default_scheduler_expert_config();
    cfg.ue.enable_csi_rs_pdsch_multiplexing = enable_csi_rs_pdsch_multiplexing;
    cfg.ue.max_msg4_mcs                     = max_msg4_mcs_index;
    cfg.ue.pdsch_interleaving_bundle_size   = pdsch_interleaving_bundle_size;
    return cfg;
  }

  sched_cell_configuration_request_message
  create_custom_cell_config_request(duplex_mode mode, bool enable_pusch_transform_precoding) const
  {
    auto msg =
        sched_config_helper::make_default_sched_cell_configuration_request(cell_config_builder_profiles::create(mode));
    msg.ul_cfg_common.init_ul_bwp.rach_cfg_common->msg3_transform_precoder = enable_pusch_transform_precoding;
    return msg;
  }

  unsigned pdsch_tbs_scheduled_bytes_per_lc(const sched_test_ue& u, lcid_t lcid)
  {
    unsigned total_cw_tb_size_bytes = 0;
    for (const auto& grant : bench->sched_res->dl.ue_grants) {
      if (grant.pdsch_cfg.rnti != u.crnti) {
        continue;
      }
      for (const auto& tb : grant.tb_list) {
        for (const auto& s_pdu : tb.lc_chs_to_sched) {
          if (s_pdu.lcid == lcid) {
            total_cw_tb_size_bytes += s_pdu.sched_bytes;
          }
        }
      }
    }
    return total_cw_tb_size_bytes;
  }

  unsigned pusch_tbs_scheduled_bytes(const sched_test_ue& u)
  {
    unsigned total_cw_tb_size_bytes = 0;
    for (const auto& grant : bench->sched_res->ul.puschs) {
      if (grant.pusch_cfg.rnti != u.crnti) {
        continue;
      }
      total_cw_tb_size_bytes += grant.pusch_cfg.tb_size_bytes;
    }
    return total_cw_tb_size_bytes;
  }

  void add_ue(du_ue_index_t ue_index,
              lcid_t        lcid_,
              lcg_id_t      lcgid_,
              duplex_mode   mode,
              bool          enable_pusch_transform_precoding,
              bool          is_fallback = false)
  {
    const auto& cell_cfg_params = cell_config_builder_profiles::create(mode);
    add_ue(ue_index, lcid_, lcgid_, cell_cfg_params, enable_pusch_transform_precoding, is_fallback);
  }

  void add_ue(du_ue_index_t                     ue_index,
              lcid_t                            lcid_,
              lcg_id_t                          lcgid_,
              const cell_config_builder_params& params,
              bool                              enable_pusch_transform_precoding,
              bool                              is_fallback = false)
  {
    auto ue_creation_req               = sched_config_helper::create_default_sched_ue_creation_request(params);
    ue_creation_req.starts_in_fallback = is_fallback;

    ue_creation_req.ue_index = ue_index;
    ue_creation_req.crnti    = to_rnti(allocate_rnti());
    (*ue_creation_req.cfg.cells)[0].serv_cell_cfg.init_dl_bwp.pdsch_cfg->vrb_to_prb_interleaving =
        bench.value().expert_cfg.ue.pdsch_interleaving_bundle_size;

    auto it = std::find_if(ue_creation_req.cfg.lc_config_list->begin(),
                           ue_creation_req.cfg.lc_config_list->end(),
                           [lcid_](const auto& l) { return l.lcid == lcid_; });
    if (it == ue_creation_req.cfg.lc_config_list->end()) {
      ue_creation_req.cfg.lc_config_list->push_back(config_helpers::create_default_logical_channel_config(lcid_));
      it = ue_creation_req.cfg.lc_config_list->end() - 1;
    }
    it->lc_group = lcgid_;

    pucch_cfg_builder.add_build_new_ue_pucch_cfg(ue_creation_req.cfg.cells.value()[0].serv_cell_cfg);
    if (enable_pusch_transform_precoding) {
      ue_creation_req.cfg.cells.value()[0]
          .serv_cell_cfg.ul_config.value()
          .init_ul_bwp.pusch_cfg.value()
          .trans_precoder = ocudu::pusch_config::transform_precoder::enabled;
    }
    bench->sch.handle_ue_creation_request(ue_creation_req);

    bench->ues[ue_index] = sched_test_ue{ue_creation_req.crnti, {}, {}, ue_creation_req};
  }

  void add_ue(sched_ue_creation_request_message& ue_create_req, bool enable_pusch_transform_precoding)
  {
    pucch_cfg_builder.add_build_new_ue_pucch_cfg(ue_create_req.cfg.cells.value()[0].serv_cell_cfg);
    (*ue_create_req.cfg.cells)[0].serv_cell_cfg.init_dl_bwp.pdsch_cfg->vrb_to_prb_interleaving =
        bench.value().expert_cfg.ue.pdsch_interleaving_bundle_size;
    if (enable_pusch_transform_precoding) {
      ue_create_req.cfg.cells.value()[0].serv_cell_cfg.ul_config.value().init_ul_bwp.pusch_cfg.value().trans_precoder =
          pusch_config::transform_precoder::enabled;
    }
    bench->sch.handle_ue_creation_request(ue_create_req);

    bench->ues[ue_create_req.ue_index] = sched_test_ue{ue_create_req.crnti, {}, {}, ue_create_req};
  }

  sched_test_ue& get_ue(du_ue_index_t ue_idx) { return bench->ues.at(ue_idx); }

  void push_buffer_state_to_dl_ue(du_ue_index_t ue_index, unsigned buffer_size, lcid_t lcid)
  {
    auto& test_ue = get_ue(ue_index);
    // Notification from upper layers of DL buffer state.
    const dl_buffer_state_indication_message msg{ue_index, lcid, buffer_size};

    // Store to keep track of DL buffer status.
    test_ue.dl_bsr_list[lcid] = msg;

    bench->sch.handle_dl_buffer_state_indication(msg);
  }

  void push_conres_mac_ce(du_ue_index_t ue_index)
  {
    // Notification from upper layers of DL buffer state.
    const dl_mac_ce_indication msg{.ue_index = ue_index, .ce_lcid = lcid_dl_sch_t::UE_CON_RES_ID};
    bench->sch.handle_dl_mac_ce_indication(msg);
  }

  void notify_ul_bsr_from_ue(du_ue_index_t ue_index, unsigned buffer_size, lcg_id_t lcg_id)
  {
    auto& test_ue = get_ue(ue_index);
    // Assumptions:
    // - Only one LCG is assumed to have data to send.
    // - BSR is Short BSR.
    ul_bsr_indication_message msg{to_du_cell_index(0), ue_index, test_ue.crnti, bsr_format::SHORT_BSR, {}};
    msg.reported_lcgs.push_back(ul_bsr_lcg_report{lcg_id, buffer_size});

    // Store to keep track of current buffer status in UE.
    test_ue.ul_bsr_list[lcg_id] = ul_bsr_lcg_report{lcg_id, buffer_size};

    bench->sch.handle_ul_bsr_indication(msg);
  }

  const pdcch_dl_information* find_ue_dl_pdcch(const sched_test_ue& u) const
  {
    const pdcch_dl_information* it = std::find_if(bench->sched_res->dl.dl_pdcchs.begin(),
                                                  bench->sched_res->dl.dl_pdcchs.end(),
                                                  [&u](const auto& grant) { return grant.ctx.rnti == u.crnti; });
    return it == bench->sched_res->dl.dl_pdcchs.end() ? nullptr : &*it;
  }

  const pdcch_ul_information* find_ue_ul_pdcch(const sched_test_ue& u) const
  {
    const pdcch_ul_information* it = std::find_if(bench->sched_res->dl.ul_pdcchs.begin(),
                                                  bench->sched_res->dl.ul_pdcchs.end(),
                                                  [&u](const auto& grant) { return grant.ctx.rnti == u.crnti; });
    return it == bench->sched_res->dl.ul_pdcchs.end() ? nullptr : &*it;
  }

  const dl_msg_alloc* find_ue_pdsch(const sched_test_ue& u) const
  {
    const auto* it = std::find_if(bench->sched_res->dl.ue_grants.begin(),
                                  bench->sched_res->dl.ue_grants.end(),
                                  [&u](const auto& grant) { return grant.pdsch_cfg.rnti == u.crnti; });
    return it == bench->sched_res->dl.ue_grants.end() ? nullptr : &*it;
  }

  bool ue_is_allocated_pdsch(const sched_test_ue& u) const { return find_ue_pdsch(u) != nullptr; }

  const ul_sched_info* find_ue_pusch(const sched_test_ue& u) const
  {
    const auto* it = std::find_if(bench->sched_res->ul.puschs.begin(),
                                  bench->sched_res->ul.puschs.end(),
                                  [&u](const auto& grant) { return grant.pusch_cfg.rnti == u.crnti; });
    return it == bench->sched_res->ul.puschs.end() ? nullptr : &*it;
  }

  bool ue_is_allocated_pusch(const sched_test_ue& u) const { return find_ue_pusch(u) != nullptr; }

  std::optional<slot_point> get_pusch_scheduled_slot(const sched_test_ue& u) const
  {
    const pdcch_ul_information* it = std::find_if(bench->sched_res->dl.ul_pdcchs.begin(),
                                                  bench->sched_res->dl.ul_pdcchs.end(),
                                                  [&u](const auto& grant) { return grant.ctx.rnti == u.crnti; });
    const auto&                 ue_ul_lst =
        (*u.msg.cfg.cells)[0].serv_cell_cfg.ul_config.value().init_ul_bwp.pusch_cfg.value().pusch_td_alloc_list;
    const auto& cell_ul_lst = bench->cell_cfg.ul_cfg_common.init_ul_bwp.pusch_cfg_common->pusch_td_alloc_list;
    const auto& ul_lst      = ue_ul_lst.empty() ? cell_ul_lst : ue_ul_lst;

    if (it == bench->sched_res->dl.ul_pdcchs.end()) {
      return {};
    }

    switch (it->dci.type) {
      case dci_ul_rnti_config_type::tc_rnti_f0_0:
        return current_slot + ul_lst[it->dci.tc_rnti_f0_0.time_resource].k2;
      case dci_ul_rnti_config_type::c_rnti_f0_0:
        return current_slot + ul_lst[it->dci.c_rnti_f0_0.time_resource].k2;
      case dci_ul_rnti_config_type::c_rnti_f0_1:
        return current_slot + ul_lst[it->dci.c_rnti_f0_1.time_resource].k2;
    }

    return {};
  }

  std::optional<slot_point> get_pdsch_scheduled_slot(const sched_test_ue& u) const
  {
    const pdcch_dl_information* it = std::find_if(bench->sched_res->dl.dl_pdcchs.begin(),
                                                  bench->sched_res->dl.dl_pdcchs.end(),
                                                  [&u](const auto& grant) { return grant.ctx.rnti == u.crnti; });
    const auto& ue_dl_lst   = (*u.msg.cfg.cells)[0].serv_cell_cfg.init_dl_bwp.pdsch_cfg.value().pdsch_td_alloc_list;
    const auto& cell_dl_lst = bench->cell_cfg.dl_cfg_common.init_dl_bwp.pdsch_common.pdsch_td_alloc_list;
    const auto& dl_lst      = ue_dl_lst.empty() ? cell_dl_lst : ue_dl_lst;

    if (it == bench->sched_res->dl.dl_pdcchs.end()) {
      return {};
    }

    switch (it->dci.type) {
      case dci_dl_rnti_config_type::c_rnti_f1_0:
        return current_slot + dl_lst[it->dci.c_rnti_f1_0.time_resource].k0;
      case dci_dl_rnti_config_type::tc_rnti_f1_0:
        return current_slot + dl_lst[it->dci.tc_rnti_f1_0.time_resource].k0;
      case dci_dl_rnti_config_type::c_rnti_f1_1:
        return current_slot + dl_lst[it->dci.c_rnti_f1_1.time_resource].k0;
      default:
        break;
    }

    return {};
  }

  std::optional<slot_point> get_pdsch_ack_nack_scheduled_slot(const sched_test_ue& u) const
  {
    const pdcch_dl_information* it = std::find_if(bench->sched_res->dl.dl_pdcchs.begin(),
                                                  bench->sched_res->dl.dl_pdcchs.end(),
                                                  [&u](const auto& grant) { return grant.ctx.rnti == u.crnti; });

    if (it == bench->sched_res->dl.dl_pdcchs.end()) {
      return {};
    }

    const auto& dl_to_ack_lst =
        (*u.msg.cfg.cells)[0].serv_cell_cfg.ul_config.value().init_ul_bwp.pucch_cfg.value().dl_data_to_ul_ack;

    // TS38.213, 9.2.3 - For DCI f1_0, the PDSCH-to-HARQ-timing-indicator field values map to {1, 2, 3, 4, 5, 6, 7, 8}.
    // PDSCH-to-HARQ-timing-indicator provide the index in {1, 2, 3, 4, 5, 6, 7, 8} starting from 0 .. 7.
    switch (it->dci.type) {
      case dci_dl_rnti_config_type::c_rnti_f1_0:
        return current_slot + it->dci.c_rnti_f1_0.pdsch_harq_fb_timing_indicator + 1;
      case dci_dl_rnti_config_type::tc_rnti_f1_0:
        return current_slot + it->dci.tc_rnti_f1_0.pdsch_harq_fb_timing_indicator + 1;
      case dci_dl_rnti_config_type::c_rnti_f1_1:
        return current_slot + dl_to_ack_lst[it->dci.c_rnti_f1_1.pdsch_harq_fb_timing_indicator.value()];
      default:
        break;
    }

    return {};
  }

  uci_indication build_harq_ack_pucch_f0_f1_uci_ind(const du_ue_index_t ue_idx, slot_point sl_tx)
  {
    const sched_test_ue& u = get_ue(ue_idx);

    uci_indication::uci_pdu::uci_pucch_f0_or_f1_pdu pucch_pdu{};
    pucch_pdu.sr_detected = false;
    pucch_pdu.harqs.push_back(mac_harq_ack_report_status::ack);

    uci_indication::uci_pdu pdu{};
    pdu.crnti    = u.crnti;
    pdu.ue_index = ue_idx;
    pdu.pdu      = pucch_pdu;

    uci_indication uci_ind{};
    uci_ind.slot_rx    = sl_tx;
    uci_ind.cell_index = to_du_cell_index(0);
    uci_ind.ucis.push_back(pdu);

    return uci_ind;
  }

  uci_indication build_harq_nack_pucch_f0_f1_uci_ind(const du_ue_index_t ue_idx, slot_point sl_tx)
  {
    const sched_test_ue& u = get_ue(ue_idx);

    uci_indication::uci_pdu::uci_pucch_f0_or_f1_pdu pucch_pdu{};
    pucch_pdu.sr_detected = false;
    pucch_pdu.harqs.push_back(mac_harq_ack_report_status::nack);

    uci_indication::uci_pdu pdu{};
    pdu.crnti    = u.crnti;
    pdu.ue_index = ue_idx;
    pdu.pdu      = pucch_pdu;

    uci_indication uci_ind{};
    uci_ind.slot_rx    = sl_tx;
    uci_ind.cell_index = to_du_cell_index(0);
    uci_ind.ucis.push_back(pdu);

    return uci_ind;
  }

  uci_indication::uci_pdu build_pucch_uci_pdu(const pucch_info& pucch)
  {
    uci_indication::uci_pdu pdu{};
    pdu.crnti    = pucch.crnti;
    pdu.ue_index = bench->rnti_to_du_ue_index(pdu.crnti);

    switch (pucch.format()) {
      case pucch_format::FORMAT_0:
      case pucch_format::FORMAT_1: {
        uci_indication::uci_pdu::uci_pucch_f0_or_f1_pdu pucch_pdu{};
        const sr_nof_bits                               sr_bits           = pucch.uci_bits.sr_bits;
        const unsigned                                  harq_ack_nof_bits = pucch.uci_bits.harq_ack_nof_bits;
        pucch_pdu.sr_detected                                             = sr_nof_bits_to_uint(sr_bits) > 0;
        // Auto ACK harqs.
        pucch_pdu.harqs.resize(harq_ack_nof_bits, mac_harq_ack_report_status::ack);
        pucch_pdu.ul_sinr_dB = 55;
        pdu.pdu              = pucch_pdu;
        break;
      }
      case pucch_format::FORMAT_2: {
        uci_indication::uci_pdu::uci_pucch_f2_or_f3_or_f4_pdu pucch_pdu{};
        pucch_pdu.sr_info.resize(sr_nof_bits_to_uint(pucch.uci_bits.sr_bits));
        pucch_pdu.sr_info.fill(0, sr_nof_bits_to_uint(pucch.uci_bits.sr_bits), true);
        // Auto ACK harqs.
        pucch_pdu.harqs.resize(pucch.uci_bits.harq_ack_nof_bits, mac_harq_ack_report_status::ack);
        if (pucch.csi_rep_cfg.has_value()) {
          pucch_pdu.csi.emplace();
          // Fill with dummy values.
          pucch_pdu.csi->ri                    = 1;
          pucch_pdu.csi->first_tb_wideband_cqi = 15;
        }
        pucch_pdu.ul_sinr_dB = 55;
        pdu.pdu              = pucch_pdu;
        break;
      }
      default:
        ocudu_assertion_failure("Unsupported PUCCH format");
    }
    return pdu;
  }

  uci_indication::uci_pdu build_pusch_uci_pdu(const ul_sched_info& pusch)
  {
    uci_indication::uci_pdu pdu{};
    pdu.crnti    = pusch.pusch_cfg.rnti;
    pdu.ue_index = bench->rnti_to_du_ue_index(pdu.crnti);

    uci_indication::uci_pdu::uci_pusch_pdu pusch_pdu{};
    // Auto ACK harqs.
    if (pusch.uci->harq.has_value()) {
      pusch_pdu.harqs.resize(pusch.uci->harq->harq_ack_nof_bits, mac_harq_ack_report_status::ack);
    }
    if (pusch.uci->csi.has_value()) {
      pusch_pdu.csi.emplace();
      // Fill with dummy values.
      pusch_pdu.csi->ri                    = 1;
      pusch_pdu.csi->first_tb_wideband_cqi = 15;
    }
    pdu.pdu = pusch_pdu;
    return pdu;
  }

  ul_crc_pdu_indication build_success_crc_pdu_indication(const du_ue_index_t ue_idx, const uint8_t harq_id)
  {
    const sched_test_ue& u = get_ue(ue_idx);

    ul_crc_pdu_indication pdu{};
    pdu.ue_index       = ue_idx;
    pdu.rnti           = u.crnti;
    pdu.harq_id        = (harq_id_t)harq_id;
    pdu.tb_crc_success = true;
    pdu.ul_sinr_dB     = 55;

    return pdu;
  }

  std::optional<search_space_configuration> get_ss_cfg(const sched_test_ue& u, search_space_id ss_id)
  {
    auto it = std::find_if((*u.msg.cfg.cells)[0].serv_cell_cfg.init_dl_bwp.pdcch_cfg.value().search_spaces.begin(),
                           (*u.msg.cfg.cells)[0].serv_cell_cfg.init_dl_bwp.pdcch_cfg.value().search_spaces.end(),
                           [ss_id](const search_space_configuration& cfg) { return cfg.get_id() == ss_id; });
    if (it != (*u.msg.cfg.cells)[0].serv_cell_cfg.init_dl_bwp.pdcch_cfg.value().search_spaces.end()) {
      return *it;
    }

    it = std::find_if(bench->cell_cfg.dl_cfg_common.init_dl_bwp.pdcch_common.search_spaces.begin(),
                      bench->cell_cfg.dl_cfg_common.init_dl_bwp.pdcch_common.search_spaces.end(),
                      [ss_id](const search_space_configuration& cfg) { return cfg.get_id() == ss_id; });
    if (it != bench->cell_cfg.dl_cfg_common.init_dl_bwp.pdcch_common.search_spaces.end()) {
      return *it;
    }

    return std::nullopt;
  }
};

class single_ue_sched_tester : public scheduler_impl_tester, public ::testing::Test
{};

TEST_F(single_ue_sched_tester, successfully_schedule_srb0_retransmission_fdd)
{
  setup_sched(create_expert_config(6, vrb_to_prb::mapping_type::non_interleaved),
              create_custom_cell_config_request(duplex_mode::FDD, false));

  // Keep track of ACKs to send.
  std::optional<uci_indication> uci_ind_to_send;

  // Add UE(s) and notify UL BSR + DL Buffer status with 110 value.
  // Assumption: LCID is SRB0.
  const bool is_fallback = true;
  add_ue(to_du_ue_index(0), LCID_SRB0, static_cast<lcg_id_t>(0), duplex_mode::FDD, false, is_fallback);

  // Enqueue ConRes CE.
  bench->sch.handle_dl_mac_ce_indication(dl_mac_ce_indication{to_du_ue_index(0), lcid_dl_sch_t::UE_CON_RES_ID});

  // Notify about SRB0 message in DL of size 101 bytes.
  const unsigned mac_srb0_sdu_size = 101;
  push_buffer_state_to_dl_ue(to_du_ue_index(0), mac_srb0_sdu_size, LCID_SRB0);

  bool successfully_schedule_srb0_retransmission = false;

  for (unsigned i = 0; i != test_bench::max_test_run_slots_per_ue * get_nof_slots_per_subframe(current_slot.scs());
       ++i) {
    run_slot();

    auto&       test_ue = get_ue(to_du_ue_index(0));
    const auto* grant   = find_ue_pdsch(test_ue);
    // Re-transmission scenario.
    if (grant != nullptr && grant->context.nof_retxs > 0) {
      // Must be Type1-PDCCH CSS.
      // See 3GPP TS 38.213, clause 10.1,
      // A UE monitors PDCCH candidates in one or more of the following search spaces sets:
      //  - a Type1-PDCCH CSS set configured by ra-SearchSpace in PDCCH-ConfigCommon for a DCI format with
      //    CRC scrambled by a RA-RNTI, a MsgB-RNTI, or a TC-RNTI on the primary cell.
      ASSERT_EQ(grant->pdsch_cfg.ss_set_type, search_space_set_type::type1);
      successfully_schedule_srb0_retransmission = true;
      break;
    }

    if (uci_ind_to_send.has_value() and current_slot == uci_ind_to_send.value().slot_rx) {
      // Send the NACK indication on the expected slot to trigger retransmission.
      bench->sch.handle_uci_indication(uci_ind_to_send.value());
      uci_ind_to_send.reset();
    }

    // Look for a PDSCH in the scheduler results and get the corresponding ACK/NACK slot.
    const auto& ack_nack_slot = get_pdsch_ack_nack_scheduled_slot(test_ue);
    if (ack_nack_slot.has_value()) {
      // Build a NACK for the first PDSCH scheduled to trigger retransmission.
      uci_ind_to_send.emplace(build_harq_nack_pucch_f0_f1_uci_ind(to_du_ue_index(0), ack_nack_slot.value()));
    }
  }
  ASSERT_TRUE(successfully_schedule_srb0_retransmission) << fmt::format("SRB0 retransmission is not scheduled");
}

TEST_F(single_ue_sched_tester, srb0_retransmission_not_scheduled_if_csi_rs_is_present_fdd)
{
  // Keep track of ACKs to send.
  std::optional<uci_indication> uci_ind_to_send;

  setup_sched(create_expert_config(10, vrb_to_prb::mapping_type::non_interleaved),
              create_custom_cell_config_request(ocudu::duplex_mode::FDD, false));
  // Add UE.
  add_ue(to_du_ue_index(0), LCID_SRB0, static_cast<lcg_id_t>(0), ocudu::duplex_mode::FDD, false);

  if (not bench->cell_cfg.nzp_csi_rs_list.empty()) {
    const unsigned csi_rs_periodicity =
        bench->cell_cfg.nzp_csi_rs_list[0].csi_res_period.has_value()
            ? csi_resource_periodicity_to_uint(*bench->cell_cfg.nzp_csi_rs_list[0].csi_res_period)
            : 0;
    const unsigned csi_rs_slot_offset = bench->cell_cfg.nzp_csi_rs_list[0].csi_res_offset.has_value()
                                            ? *bench->cell_cfg.nzp_csi_rs_list[0].csi_res_offset
                                            : 0;
    const unsigned min_k1             = (*get_ue(to_du_ue_index(0)).msg.cfg.cells)[0]
                                .serv_cell_cfg.ul_config->init_ul_bwp.pucch_cfg->dl_data_to_ul_ack[0];
    auto& test_ue = get_ue(to_du_ue_index(0));
    // Flag to keep track of multiplexing status of SRB0 retransmission PDSCH and CSI-RS.
    bool is_csi_muplxed_with_srb0_retx_pdsch = false;
    // CSI-RS periodicity in slot + CSI-RS slot offset - Min. K1 in list - 1 (slot in which to schedule SRB0 new tx) - 1
    // (UL HARQ processing delay).
    const unsigned srb0_new_tx_to_csi_rs_slot_delay = csi_rs_periodicity + csi_rs_slot_offset - min_k1 - 2;

    for (unsigned i = 0; i != csi_rs_periodicity * 3; ++i) {
      // Run until the next CSI-RS period minus min_k1 slot.
      if (i < srb0_new_tx_to_csi_rs_slot_delay) {
        run_slot();
        continue;
      }
      // Push DL buffer status indication of 100 bytes srb0_new_tx_to_csi_rs_slot_delay slots before CSI-RS so that SRB0
      // retransmission falls in slot with CSI-RS.
      if (i == srb0_new_tx_to_csi_rs_slot_delay) {
        push_buffer_state_to_dl_ue(to_du_ue_index(0), 100, LCID_SRB0);
      }
      run_slot();

      const auto* grant = find_ue_pdsch(test_ue);
      // Re-transmission scenario.
      if (grant != nullptr and grant->context.nof_retxs > 0 and (not bench->sched_res->dl.csi_rs.empty())) {
        is_csi_muplxed_with_srb0_retx_pdsch = true;
      }

      if (uci_ind_to_send.has_value() and current_slot == uci_ind_to_send.value().slot_rx) {
        bench->sch.handle_uci_indication(uci_ind_to_send.value());
        uci_ind_to_send.reset();
      }

      const auto& ack_nack_slot = get_pdsch_ack_nack_scheduled_slot(test_ue);
      if (ack_nack_slot.has_value()) {
        uci_ind_to_send.emplace(build_harq_nack_pucch_f0_f1_uci_ind(to_du_ue_index(0), ack_nack_slot.value()));
      }
    }

    ASSERT_FALSE(is_csi_muplxed_with_srb0_retx_pdsch)
        << fmt::format("CSI-RS being multiplexed with SRB0 retransmission");
  }
}

TEST_F(single_ue_sched_tester, test_ue_scheduling_with_empty_spcell_cfg)
{
  setup_sched(create_expert_config(10, vrb_to_prb::mapping_type::non_interleaved),
              create_custom_cell_config_request(ocudu::duplex_mode::TDD, false));
  // Add UE.
  const auto& cell_cfg_params = cell_config_builder_profiles::create(ocudu::duplex_mode::TDD);
  auto        ue_creation_req = sched_config_helper::create_empty_spcell_cfg_sched_ue_creation_request(cell_cfg_params);
  ue_creation_req.starts_in_fallback = true;

  ue_creation_req.ue_index = to_du_ue_index(0);
  ue_creation_req.crnti    = to_rnti(allocate_rnti());
  bench->sch.handle_ue_creation_request(ue_creation_req);
  bench->ues[ue_creation_req.ue_index] = sched_test_ue{ue_creation_req.crnti, {}, {}, ue_creation_req};

  run_slot();

  // Push ConRes MAC CE to trigger the ConRes completion in the scheduler.
  push_conres_mac_ce(to_du_ue_index(0));
  // Push DL buffer status indication.
  push_buffer_state_to_dl_ue(to_du_ue_index(0), 100, LCID_SRB0);

  bool successfully_scheduled_srb0_bytes = false;
  for (unsigned i = 0; i != 20; ++i) {
    run_slot();
    auto&       test_ue = get_ue(to_du_ue_index(0));
    const auto* grant   = find_ue_pdsch(test_ue);
    if (grant != nullptr) {
      successfully_scheduled_srb0_bytes = true;
      break;
    }
  }
  ASSERT_TRUE(successfully_scheduled_srb0_bytes)
      << fmt::format("SRB0 not scheduled for UE with empty SpCell configuration");
}
