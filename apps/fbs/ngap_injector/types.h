// UHM WINGS Fake Base Station Research

#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace ocudu::fbs {

enum class harness_mode {
  passive_sniff,
  setup_replay,
  setup_construct,
  decode_only,
  negative_test
};

enum class ngap_connection_state {
  disconnected,
  sctp_connected,
  ng_setup_request_sent,
  ng_setup_accepted,
  ng_setup_failed,
  ready_for_test_messages
};

struct ue_identifiers {
  std::optional<uint64_t> ran_ue_ngap_id;
  std::optional<uint64_t> amf_ue_ngap_id;
};

struct observed_ue_context {
  std::optional<uint64_t> ran_ue_ngap_id;
  std::optional<uint64_t> amf_ue_ngap_id;
  std::string             last_message_type;
  std::string             source_ip;
  std::string             destination_ip;
  std::string             timestamp;
};

struct gnb_identity_config {
  uint32_t    id            = 411;
  unsigned    id_bit_length = 22;
  std::string ran_node_name = "ocudu-ngap-injector";
  std::string plmn          = "00101";
  uint32_t    tac           = 7;
  uint8_t     sst           = 1;
  uint16_t    default_paging_drx = 256;
};

struct negative_test_config {
  unsigned max_packet_count = 1;
  unsigned min_interval_ms  = 1000;
};

struct injector_config {
  std::string              local_gnb_ip;
  std::string              amf_ip;
  uint16_t                 sctp_port = 38412;
  std::string              interface_name;
  std::optional<std::string> pcap_path;
  ue_identifiers           manual_ue_ids;
  gnb_identity_config      gnb;
  std::vector<std::string> allowlisted_amf_ips;
  std::vector<std::string> allowlisted_interfaces;
  harness_mode             mode = harness_mode::passive_sniff;
  negative_test_config     negative_tests;
};

struct runtime_options {
  std::string config_path;
  std::string pcap_path;
  std::string context_path;
  std::string export_context_path;
  unsigned    selected_message_index = 0;
  bool        selected_message_index_set = false;
  unsigned    max_packets            = 0;
  unsigned    duration_seconds       = 0;
  bool        dry_run                = true;
  bool        dry_run_explicit       = false;
  bool        confirm_send           = false;
  bool        enable_negative_tests  = false;
  bool        lab_override           = false;
  std::string lab_override_confirm;
};

const char* to_string(harness_mode mode);
const char* to_string(ngap_connection_state state);

} // namespace ocudu::fbs
