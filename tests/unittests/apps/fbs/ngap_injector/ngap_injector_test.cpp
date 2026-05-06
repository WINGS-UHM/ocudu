// UHM WINGS Fake Base Station Research

#include "config.h"
#include "negative_tests.h"
#include "ngap_decoder.h"
#include "pcap_parser.h"
#include "state_machine.h"
#include <fstream>
#include <gtest/gtest.h>
#include <vector>

using namespace ocudu::fbs;

namespace {

injector_config valid_config()
{
  injector_config cfg;
  cfg.local_gnb_ip            = "10.10.0.2";
  cfg.amf_ip                  = "10.10.0.1";
  cfg.interface_name          = "n2-lab0";
  cfg.allowlisted_amf_ips     = {"10.10.0.1"};
  cfg.allowlisted_interfaces  = {"n2-lab0"};
  cfg.manual_ue_ids.ran_ue_ngap_id = 7;
  cfg.manual_ue_ids.amf_ue_ngap_id = 9;
  return cfg;
}

std::string write_config_file()
{
  const std::string path = "/tmp/ngap_injector_test_config.yaml";
  std::ofstream     f(path);
  f << "local_gnb_ip: 10.10.0.2\n";
  f << "amf_ip: 10.10.0.1\n";
  f << "sctp_port: 38412\n";
  f << "interface: n2-lab0\n";
  f << "allowlisted_amf_ips: [10.10.0.1]\n";
  f << "allowlisted_interfaces: [n2-lab0]\n";
  f << "mode: passive-sniff\n";
  f << "ue:\n";
  f << "  ran_ue_ngap_id: 7\n";
  f << "  amf_ue_ngap_id: 9\n";
  f << "gnb:\n";
  f << "  id: 411\n";
  f << "  id_bit_length: 22\n";
  f << "  ran_node_name: tstgnb01\n";
  f << "  plmn: '00101'\n";
  f << "  tac: 7\n";
  f << "  sst: 1\n";
  return path;
}

void append_be16(std::vector<uint8_t>& bytes, uint16_t value)
{
  bytes.push_back(static_cast<uint8_t>((value >> 8U) & 0xffU));
  bytes.push_back(static_cast<uint8_t>(value & 0xffU));
}

void append_be32(std::vector<uint8_t>& bytes, uint32_t value)
{
  bytes.push_back(static_cast<uint8_t>((value >> 24U) & 0xffU));
  bytes.push_back(static_cast<uint8_t>((value >> 16U) & 0xffU));
  bytes.push_back(static_cast<uint8_t>((value >> 8U) & 0xffU));
  bytes.push_back(static_cast<uint8_t>(value & 0xffU));
}

std::vector<uint8_t> build_sctp_ngap_ethernet_frame(const std::vector<uint8_t>& payload)
{
  std::vector<uint8_t> frame(14, 0);
  frame[12] = 0x08;
  frame[13] = 0x00;

  const uint16_t ip_total_len = static_cast<uint16_t>(20 + 12 + 16 + payload.size());
  frame.push_back(0x45);
  frame.push_back(0x00);
  append_be16(frame, ip_total_len);
  append_be16(frame, 0);
  append_be16(frame, 0);
  frame.push_back(64);
  frame.push_back(132);
  append_be16(frame, 0);
  frame.insert(frame.end(), {10, 10, 0, 2});
  frame.insert(frame.end(), {10, 10, 0, 1});

  append_be16(frame, 38412);
  append_be16(frame, 38412);
  append_be32(frame, 0);
  append_be32(frame, 0);

  frame.push_back(0);
  frame.push_back(0x03);
  append_be16(frame, static_cast<uint16_t>(16 + payload.size()));
  append_be32(frame, 1);
  append_be16(frame, 0);
  append_be16(frame, 0);
  append_be32(frame, 60);
  frame.insert(frame.end(), payload.begin(), payload.end());
  return frame;
}

} // namespace

TEST(ngap_injector_config, parses_yaml_and_validates_allowlists)
{
  injector_config cfg = load_injector_config(write_config_file());
  ASSERT_EQ(cfg.local_gnb_ip, "10.10.0.2");
  ASSERT_EQ(cfg.amf_ip, "10.10.0.1");
  ASSERT_TRUE(is_amf_allowlisted(cfg));
  ASSERT_TRUE(is_interface_allowlisted(cfg));
}

TEST(ngap_injector_config, refuses_non_allowlisted_amf)
{
  injector_config cfg = valid_config();
  cfg.amf_ip          = "10.10.0.99";
  runtime_options opts;
  ASSERT_THROW(validate_config_for_mode(cfg, opts, harness_mode::setup_construct), std::runtime_error);
}

TEST(ngap_injector_config, classifies_private_and_public_ips)
{
  ASSERT_TRUE(is_private_or_lab_scoped_ip("10.0.0.1"));
  ASSERT_TRUE(is_private_or_lab_scoped_ip("192.168.10.20"));
  ASSERT_FALSE(is_private_or_lab_scoped_ip("8.8.8.8"));
}

TEST(ngap_injector_decoder, summarizes_constructed_ng_setup_request)
{
  injector_config cfg     = valid_config();
  byte_buffer     request = build_ng_setup_request(cfg);
  auto            summary = decode_ngap_payload(request);
  ASSERT_TRUE(summary.decode_ok);
  ASSERT_TRUE(is_ng_setup_request(summary));
  ASSERT_TRUE(summary.gnb_id.has_value());
  ASSERT_EQ(*summary.gnb_id, 411U);
  ASSERT_EQ(summary.plmn, "00101");
}

TEST(ngap_injector_pcap, extracts_ngap_payload_from_sctp_data_chunk)
{
  const std::vector<uint8_t> payload = {0x00, 0x15, 0x00};
  const auto frame = build_sctp_ngap_ethernet_frame(payload);
  auto packets = extract_ngap_payloads_from_packet(frame.data(), frame.size(), 1);
  ASSERT_EQ(packets.size(), 1);
  ASSERT_EQ(packets.front().source_ip, "10.10.0.2");
  ASSERT_EQ(packets.front().destination_ip, "10.10.0.1");
  ASSERT_EQ(packets.front().payload, payload);
}

TEST(ngap_injector_state_machine, enforces_expected_setup_order)
{
  ngap_state_machine states;
  ASSERT_TRUE(states.can_transition_to(ngap_connection_state::sctp_connected));
  states.transition_to(ngap_connection_state::sctp_connected);
  ASSERT_TRUE(states.can_transition_to(ngap_connection_state::ng_setup_request_sent));
  ASSERT_FALSE(states.can_transition_to(ngap_connection_state::ready_for_test_messages));
}

TEST(ngap_injector_negative_tests, dry_run_ue_context_release_does_not_require_enable_flag)
{
  injector_config cfg = valid_config();
  runtime_options opts;
  opts.dry_run = true;
  ASSERT_NO_THROW(run_negative_ue_context_release(cfg, opts));
}

TEST(ngap_injector_negative_tests, actual_send_requires_enable_flag)
{
  injector_config cfg = valid_config();
  runtime_options opts;
  opts.dry_run      = false;
  opts.confirm_send = true;
  ASSERT_THROW(run_negative_ue_context_release(cfg, opts), std::runtime_error);
}
