// UHM WINGS Fake Base Station Research

#include "config.h"
#include "ocudu/gateways/sctp_network_gateway.h"
#include <algorithm>
#include <arpa/inet.h>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <yaml-cpp/yaml.h>

using namespace ocudu::fbs;

namespace {

template <typename T>
T read_scalar_or(const YAML::Node& node, const char* key, T default_value)
{
  if (node && node[key]) {
    return node[key].as<T>();
  }
  return default_value;
}

template <typename T>
std::optional<T> read_optional_scalar(const YAML::Node& node, const char* key)
{
  if (node && node[key]) {
    return node[key].as<T>();
  }
  return std::nullopt;
}

std::vector<std::string> read_string_vector(const YAML::Node& node)
{
  std::vector<std::string> values;
  if (!node) {
    return values;
  }
  if (node.IsSequence()) {
    for (const auto& item : node) {
      values.push_back(item.as<std::string>());
    }
  } else if (node.IsScalar()) {
    values.push_back(node.as<std::string>());
  }
  return values;
}

bool contains_value(const std::vector<std::string>& values, const std::string& needle)
{
  return std::find(values.begin(), values.end(), needle) != values.end();
}

uint32_t ipv4_to_host_order(const std::string& value)
{
  in_addr addr = {};
  if (::inet_pton(AF_INET, value.c_str(), &addr) != 1) {
    return 0;
  }
  return ntohl(addr.s_addr);
}

bool is_ipv6_private_or_lab(const std::string& value)
{
  in6_addr addr = {};
  if (::inet_pton(AF_INET6, value.c_str(), &addr) != 1) {
    return false;
  }

  const uint8_t first  = addr.s6_addr[0];
  const uint8_t second = addr.s6_addr[1];

  // fc00::/7 unique-local, fe80::/10 link-local, ::1 loopback.
  if ((first & 0xfeU) == 0xfcU || (first == 0xfeU && (second & 0xc0U) == 0x80U)) {
    return true;
  }
  static constexpr uint8_t loopback[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
  return std::memcmp(addr.s6_addr, loopback, sizeof(loopback)) == 0;
}

void require_field(bool ok, const char* field)
{
  if (!ok) {
    throw std::runtime_error(std::string("Missing required NGAP injector config field: ") + field);
  }
}

} // namespace

const char* ocudu::fbs::to_string(harness_mode mode)
{
  switch (mode) {
    case harness_mode::passive_sniff:
      return "passive-sniff";
    case harness_mode::setup_replay:
      return "setup-replay";
    case harness_mode::setup_construct:
      return "setup-construct";
    case harness_mode::decode_only:
      return "decode-only";
    case harness_mode::negative_test:
      return "negative-test";
  }
  return "unknown";
}

const char* ocudu::fbs::to_string(ngap_connection_state state)
{
  switch (state) {
    case ngap_connection_state::disconnected:
      return "DISCONNECTED";
    case ngap_connection_state::sctp_connected:
      return "SCTP_CONNECTED";
    case ngap_connection_state::ng_setup_request_sent:
      return "NG_SETUP_REQUEST_SENT";
    case ngap_connection_state::ng_setup_accepted:
      return "NG_SETUP_ACCEPTED";
    case ngap_connection_state::ng_setup_failed:
      return "NG_SETUP_FAILED";
    case ngap_connection_state::ready_for_test_messages:
      return "READY_FOR_TEST_MESSAGES";
  }
  return "UNKNOWN";
}

harness_mode ocudu::fbs::harness_mode_from_string(const std::string& value)
{
  if (value == "passive-sniff") {
    return harness_mode::passive_sniff;
  }
  if (value == "setup-replay") {
    return harness_mode::setup_replay;
  }
  if (value == "setup-construct") {
    return harness_mode::setup_construct;
  }
  if (value == "decode-only") {
    return harness_mode::decode_only;
  }
  if (value == "negative-test") {
    return harness_mode::negative_test;
  }
  throw std::runtime_error("Unsupported NGAP injector runtime mode: " + value);
}

injector_config ocudu::fbs::load_injector_config(const std::string& path)
{
  YAML::Node root = YAML::LoadFile(path);

  injector_config cfg;
  cfg.local_gnb_ip   = read_scalar_or<std::string>(root, "local_gnb_ip", "");
  cfg.amf_ip         = read_scalar_or<std::string>(root, "amf_ip", "");
  cfg.sctp_port      = read_scalar_or<uint16_t>(root, "sctp_port", ocudu::NGAP_PORT);
  cfg.interface_name = read_scalar_or<std::string>(root, "interface", "");

  if (root["local"]) {
    cfg.local_gnb_ip = read_scalar_or<std::string>(root["local"], "gnb_ip", cfg.local_gnb_ip);
    cfg.local_gnb_ip = read_scalar_or<std::string>(root["local"], "ip", cfg.local_gnb_ip);
  }
  if (root["amf"]) {
    cfg.amf_ip    = read_scalar_or<std::string>(root["amf"], "ip", cfg.amf_ip);
    cfg.sctp_port = read_scalar_or<uint16_t>(root["amf"], "sctp_port", cfg.sctp_port);
    cfg.sctp_port = read_scalar_or<uint16_t>(root["amf"], "port", cfg.sctp_port);
  }
  if (root["sniff"]) {
    cfg.interface_name = read_scalar_or<std::string>(root["sniff"], "interface", cfg.interface_name);
  }

  cfg.pcap_path = read_optional_scalar<std::string>(root, "pcap_path");
  if (root["pcap"]) {
    cfg.pcap_path = read_optional_scalar<std::string>(root["pcap"], "path");
  }

  if (auto mode = read_optional_scalar<std::string>(root, "mode")) {
    cfg.mode = harness_mode_from_string(*mode);
  }

  if (root["ue"]) {
    cfg.manual_ue_ids.ran_ue_ngap_id = read_optional_scalar<uint64_t>(root["ue"], "ran_ue_ngap_id");
    cfg.manual_ue_ids.amf_ue_ngap_id = read_optional_scalar<uint64_t>(root["ue"], "amf_ue_ngap_id");
  }

  if (root["gnb"]) {
    const YAML::Node gnb = root["gnb"];
    cfg.gnb.id          = read_scalar_or<uint32_t>(gnb, "id", cfg.gnb.id);
    cfg.gnb.id_bit_length = read_scalar_or<unsigned>(gnb, "id_bit_length", cfg.gnb.id_bit_length);
    cfg.gnb.ran_node_name = read_scalar_or<std::string>(gnb, "ran_node_name", cfg.gnb.ran_node_name);
    cfg.gnb.plmn          = read_scalar_or<std::string>(gnb, "plmn", cfg.gnb.plmn);
    cfg.gnb.tac           = read_scalar_or<uint32_t>(gnb, "tac", cfg.gnb.tac);
    cfg.gnb.sst = static_cast<uint8_t>(read_scalar_or<unsigned>(gnb, "sst", cfg.gnb.sst));
    cfg.gnb.default_paging_drx =
        read_scalar_or<uint16_t>(gnb, "default_paging_drx", cfg.gnb.default_paging_drx);
  }

  if (root["allowlisted_amf_ips"]) {
    cfg.allowlisted_amf_ips = read_string_vector(root["allowlisted_amf_ips"]);
  }
  if (root["allowlisted_interfaces"]) {
    cfg.allowlisted_interfaces = read_string_vector(root["allowlisted_interfaces"]);
  }
  if (root["allowlist"]) {
    if (root["allowlist"]["amf_ips"]) {
      cfg.allowlisted_amf_ips = read_string_vector(root["allowlist"]["amf_ips"]);
    }
    if (root["allowlist"]["interfaces"]) {
      cfg.allowlisted_interfaces = read_string_vector(root["allowlist"]["interfaces"]);
    }
  }

  if (root["negative_tests"]) {
    cfg.negative_tests.max_packet_count =
        read_scalar_or<unsigned>(root["negative_tests"], "max_packet_count", cfg.negative_tests.max_packet_count);
    cfg.negative_tests.min_interval_ms =
        read_scalar_or<unsigned>(root["negative_tests"], "min_interval_ms", cfg.negative_tests.min_interval_ms);
  }

  require_field(!cfg.local_gnb_ip.empty(), "local_gnb_ip");
  require_field(!cfg.amf_ip.empty(), "amf_ip");
  require_field(!cfg.interface_name.empty(), "interface");
  require_field(!cfg.allowlisted_amf_ips.empty(), "allowlisted_amf_ips");
  require_field(!cfg.allowlisted_interfaces.empty(), "allowlisted_interfaces");

  return cfg;
}

bool ocudu::fbs::is_valid_ip_literal(const std::string& value)
{
  in_addr  addr4 = {};
  in6_addr addr6 = {};
  return ::inet_pton(AF_INET, value.c_str(), &addr4) == 1 || ::inet_pton(AF_INET6, value.c_str(), &addr6) == 1;
}

bool ocudu::fbs::is_private_or_lab_scoped_ip(const std::string& value)
{
  const uint32_t ipv4 = ipv4_to_host_order(value);
  if (ipv4 != 0) {
    const uint8_t first  = static_cast<uint8_t>((ipv4 >> 24U) & 0xffU);
    const uint8_t second = static_cast<uint8_t>((ipv4 >> 16U) & 0xffU);
    if (first == 10 || first == 127) {
      return true;
    }
    if (first == 172 && second >= 16 && second <= 31) {
      return true;
    }
    if (first == 192 && second == 168) {
      return true;
    }
    if (first == 100 && second >= 64 && second <= 127) {
      return true;
    }
    if (first == 169 && second == 254) {
      return true;
    }
    return false;
  }
  return is_ipv6_private_or_lab(value);
}

bool ocudu::fbs::is_amf_allowlisted(const injector_config& cfg)
{
  return contains_value(cfg.allowlisted_amf_ips, cfg.amf_ip);
}

bool ocudu::fbs::is_interface_allowlisted(const injector_config& cfg)
{
  return contains_value(cfg.allowlisted_interfaces, cfg.interface_name);
}

void ocudu::fbs::validate_config_for_mode(const injector_config& cfg, const runtime_options& opts, harness_mode mode)
{
  if (!is_valid_ip_literal(cfg.local_gnb_ip)) {
    throw std::runtime_error("Configured local_gnb_ip must be an IP literal; hostname resolution is intentionally not supported");
  }
  if (!is_valid_ip_literal(cfg.amf_ip)) {
    throw std::runtime_error("Configured amf_ip must be an IP literal; hostname resolution is intentionally not supported");
  }
  if (!is_amf_allowlisted(cfg)) {
    throw std::runtime_error("Refusing to run: configured AMF IP is not present in allowlisted_amf_ips");
  }
  if (!is_interface_allowlisted(cfg)) {
    throw std::runtime_error("Refusing to run: configured interface is not present in allowlisted_interfaces");
  }
  if (!is_private_or_lab_scoped_ip(cfg.amf_ip)) {
    if (!opts.lab_override || opts.lab_override_confirm != lab_override_confirmation) {
      throw std::runtime_error(std::string("Refusing non-private AMF IP without --lab-override and --lab-override-confirm ") +
                               lab_override_confirmation);
    }
  }
  if (mode == harness_mode::negative_test && cfg.negative_tests.max_packet_count == 0) {
    throw std::runtime_error("negative_tests.max_packet_count must be greater than zero");
  }
}

void ocudu::fbs::print_run_banner(const injector_config& cfg,
                                  const runtime_options& opts,
                                  harness_mode           mode,
                                  bool                   sending_enabled)
{
  std::printf("==== NGAP research harness: authorized closed-lab use only ====\n");
  std::printf("mode=%s\n", to_string(mode));
  std::printf("config_path=%s\n", opts.config_path.c_str());
  std::printf("interface=%s\n", cfg.interface_name.c_str());
  std::printf("local_gnb_ip=%s\n", cfg.local_gnb_ip.c_str());
  std::printf("amf_ip=%s\n", cfg.amf_ip.c_str());
  std::printf("sctp_port=%u\n", cfg.sctp_port);
  std::printf("sending_enabled=%s\n", sending_enabled ? "true" : "false");
  std::printf("negative_tests_enabled=%s\n", opts.enable_negative_tests ? "true" : "false");
  std::printf("dry_run=%s\n", opts.dry_run ? "true" : "false");
  std::printf("===============================================================\n");
}
