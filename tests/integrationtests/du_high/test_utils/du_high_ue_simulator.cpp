/*
 *
 * Copyright 2021-2025 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "du_high_ue_simulator.h"
#include "du_high_env_simulator.h"
#include "tests/test_doubles/mac/mac_test_messages.h"
#include "ocudu/du/du_high/du_high_configuration.h"
#include "ocudu/mac/mac_clock_controller.h"
#include "ocudu/pcap/rlc_pcap.h"
#include "ocudu/rlc/rlc_factory.h"
#include "ocudu/rlc/rlc_srb_config_factory.h"
#include "ocudu/support/executors/inline_task_executor.h"

using namespace ocudu;

namespace {

null_rlc_pcap        pcap_sink;
inline_task_executor ue_worker;

struct mac_sdu {
  lcid_t      lcid;
  byte_buffer payload;
};

std::vector<mac_sdu> parse_mac_sdus(span<const uint8_t> pdu_buffer)
{
  std::vector<mac_sdu> sdus;

  size_t offset = 0;
  while (offset < pdu_buffer.size()) {
    uint8_t subheader_byte = pdu_buffer[offset];
    bool    F_bit          = (subheader_byte & 0x80) != 0;
    lcid_t  lcid           = static_cast<lcid_t>(subheader_byte & 0x3F);
    size_t  subheader_len  = 1;

    unsigned sdu_length = 0;
    if ((subheader_byte & 0x40) != 0) {
      // 16-bit L field
      subheader_len += 2;
      sdu_length = static_cast<unsigned>(pdu_buffer[offset + 1]) | (static_cast<unsigned>(pdu_buffer[offset + 2]) << 8);
    } else {
      // 8-bit L field
      subheader_len += 1;
      sdu_length = static_cast<unsigned>(pdu_buffer[offset + 1]);
    }

    offset += subheader_len;
    if (offset + sdu_length > pdu_buffer.size()) {
      // Invalid PDU
      break;
    }

    mac_sdu sdu;
    sdu.lcid                       = lcid;
    span<const uint8_t> sdupayload = pdu_buffer.subspan(offset, sdu_length);
    sdu.payload                    = byte_buffer::create(sdupayload.begin(), sdupayload.end()).value();
    sdus.push_back(std::move(sdu));

    offset += sdu_length;

    if (!F_bit) {
      break;
    }
  }

  return sdus;
}

} // namespace

class du_high_ue_simulator::rlc_bearer_adapter : public rlc_tx_lower_layer_notifier,
                                                 public rlc_tx_upper_layer_control_notifier,
                                                 public rlc_tx_upper_layer_data_notifier,
                                                 public rlc_rx_upper_layer_data_notifier
{
public:
  void on_buffer_state_update(const rlc_buffer_state& bsr) override { last_reported_dl_bo = bsr; }

  void on_protocol_failure() override {}
  void on_max_retx() override {}

  void on_transmitted_sdu(uint32_t max_tx_pdcp_sn, uint32_t desired_buf_size) override {}
  void on_delivered_sdu(uint32_t max_deliv_pdcp_sn) override {}
  void on_retransmitted_sdu(uint32_t max_retx_pdcp_sn) override {}
  void on_delivered_retransmitted_sdu(uint32_t max_deliv_retx_pdcp_sn) override {}

  void on_new_sdu(byte_buffer_chain pdu) override {}

  std::optional<rlc_buffer_state> last_reported_dl_bo;
};

du_high_ue_simulator::du_high_ue_simulator(const du_high_ue_simulator_config& cfg_,
                                           const odu::du_high_dependencies&   du_hi_deps_) :
  cfg(cfg_)
{
  // Create SRB entities upfront.
  std::map<srb_id_t, odu::du_srb_config> srb_cfgs = cfg.du_high_cfg.ran.srbs;
  if (srb_cfgs.count(srb_id_t::srb0) == 0) {
    srb_cfgs[srb_id_t::srb0].rlc = make_default_srb0_rlc_config();
  }
  if (srb_cfgs.count(srb_id_t::srb1) == 0) {
    srb_cfgs[srb_id_t::srb1].rlc = make_default_srb_rlc_config();
  }
  if (srb_cfgs.count(srb_id_t::srb2) == 0) {
    srb_cfgs[srb_id_t::srb2].rlc = make_default_srb_rlc_config();
  }

  for (const auto& [srb_id, srb_cfg] : srb_cfgs) {
    bearer_context bc;
    bc.adapter = std::make_unique<rlc_bearer_adapter>();
    {
      rlc_entity_creation_message msg;
      msg.gnb_du_id         = gnb_du_id_t::max; // Reserve max DU-ID for UE RLC entities.
      msg.ue_index          = to_du_ue_index(static_cast<unsigned>(cfg.crnti) % MAX_NOF_DU_UES);
      msg.rb_id             = rb_id_t{srb_id};
      msg.config            = srb_cfg.rlc;
      msg.rx_upper_dn       = bc.adapter.get();
      msg.tx_upper_dn       = bc.adapter.get();
      msg.tx_upper_cn       = bc.adapter.get();
      msg.tx_lower_dn       = bc.adapter.get();
      msg.timers            = &du_hi_deps_.timer_ctrl->get_timer_manager();
      msg.pcell_executor    = &ue_worker;
      msg.ue_executor       = &ue_worker;
      msg.rlc_metrics_notif = nullptr;
      msg.pcap_writer       = &pcap_sink;
      auto entity           = create_rlc_entity(msg);
      bc.rlc                = std::move(entity);
    }
    auto ret = bearers.insert(std::make_pair(srb_id_to_lcid(srb_id), std::move(bc)));
    ocudu_assert(ret.second, "Failed to create RLC entity for SRB {}", fmt::underlying(srb_id));
  }
}

du_high_ue_simulator::~du_high_ue_simulator() {}

void du_high_ue_simulator::handle_dl_pdu(const mac_dl_data_result::dl_pdu& pdu)
{
  auto sdus = parse_mac_sdus(pdu.pdu.get_buffer());

  for (const auto& sdu : sdus) {
    if (not is_srb(sdu.lcid)) {
      // For now, just handle SRBs.
      continue;
    }

    auto it = bearers.find(sdu.lcid);
    ocudu_assert(
        it != bearers.end(), "Received DL PDU for LCID {} without existing RLC entity", fmt::underlying(sdu.lcid));
    it->second.rlc->get_rx_lower_layer_interface()->handle_pdu(sdu.payload);
  }
}

std::optional<byte_buffer> du_high_ue_simulator::build_next_ul_mac_pdu()
{
  std::optional<byte_buffer> pdu;
  for (const auto& [lcid, bearer] : bearers) {
    while (bearer.adapter->last_reported_dl_bo.has_value() and bearer.adapter->last_reported_dl_bo->pending_bytes > 0) {
      std::vector<uint8_t> buffer(bearer.adapter->last_reported_dl_bo->pending_bytes);
      size_t               n = bearer.rlc->get_tx_lower_layer_interface()->pull_pdu(buffer);
      if (n == 0) {
        // No PDU to transmit.
        break;
      }
      pdu = test_helpers::prepend_mac_subheader(lcid, byte_buffer::create(span<uint8_t>(buffer.data(), n)).value());
      *bearer.adapter->last_reported_dl_bo = bearer.rlc->get_tx_lower_layer_interface()->get_buffer_state();
    }
  }
  return pdu;
}

void du_high_ue_simulator::enqueue_ul_mac_sdu(lcid_t lcid, byte_buffer ul_mac_sdu)
{
  auto it = bearers.find(lcid);
  report_fatal_error_if_not(it != bearers.end(), "No RLC entity for LCID {}", fmt::underlying(lcid));
  it->second.rlc->get_tx_upper_layer_data_interface()->handle_sdu(std::move(ul_mac_sdu), false);
}
