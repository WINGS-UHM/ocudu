// UHM WINGS Fake Base Station Research

#include "CLI/CLI11.hpp"
#include "lib/gateways/sctp_network_gateway_common_impl.h"
#include "lib/ngap/ngap_asn1_helpers.h"
#include "ocudu/asn1/asn1_utils.h"
#include "ocudu/asn1/ngap/common.h"
#include "ocudu/gateways/sctp_network_gateway.h"
#include "ocudu/gateways/sctp_socket.h"
#include "ocudu/ngap/ngap_context.h"
#include "ocudu/ngap/ngap_message.h"
#include "ocudu/ocudulog/ocudulog.h"
#include "ocudu/ran/plmn_identity.h"
#include "ocudu/support/io/sockets.h"
#include <algorithm>
#include <arpa/inet.h>
#include <array>
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <netdb.h>
#include <netinet/sctp.h>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <vector>

using namespace ocudu;
using namespace ocudu::ocucp;

namespace {

static constexpr unsigned stream_no                 = 0;
static constexpr size_t   network_gateway_sctp_mtu = 9100;

struct probe_config {
  std::vector<std::string> amf_addresses;
  int                      amf_port              = NGAP_PORT;
  std::vector<std::string> bind_addresses;
  std::string              bind_interface        = "auto";
  int                      init_max_attempts     = 2;
  int                      max_init_timeo_ms     = 1000;
  int                      post_send_wait_ms     = 100;
  bool                     dump_pdu_hex          = true;
};

static byte_buffer build_ng_setup_request()
{
  ngap_context_t ngap_ctxt = {{411, 22},
                              "ocucp01",
                              "AMF",
                              amf_index_t::min,
                              {{7, {{plmn_identity::test_value(), {{slice_service_type{1}}}}}}},
                              {},
                              256};

  ngap_message ngap_msg = {};
  ngap_msg.pdu.set_init_msg();
  ngap_msg.pdu.init_msg().load_info_obj(ASN1_NGAP_ID_NG_SETUP);
  fill_asn1_ng_setup_request(ngap_msg.pdu.init_msg().value.ng_setup_request(), ngap_ctxt);

  byte_buffer   packed_pdu{byte_buffer::fallback_allocation_tag{}};
  asn1::bit_ref bref(packed_pdu);
  if (ngap_msg.pdu.pack(bref) != asn1::OCUDUASN_SUCCESS) {
    throw std::runtime_error("Failed to pack NGSetupRequest");
  }

  return packed_pdu;
}

static std::string join_strings(const std::vector<std::string>& values, const char* separator)
{
  std::string result;
  for (unsigned i = 0; i != values.size(); ++i) {
    if (i != 0) {
      result += separator;
    }
    result += values[i];
  }
  return result;
}

static std::string sockaddr_to_string(const sockaddr& addr, socklen_t addr_len)
{
  const auto info = get_nameinfo(addr, addr_len);
  return info.address + ":" + std::to_string(info.port);
}

static socklen_t sockaddr_len(const sockaddr_storage& addr)
{
  return reinterpret_cast<const sockaddr*>(&addr)->sa_family == AF_INET ? sizeof(sockaddr_in) : sizeof(sockaddr_in6);
}

static std::vector<sockaddr_storage>
resolve_sctp_addresses(const std::vector<std::string>& addresses, int port, ocudulog::basic_logger& logger)
{
  std::vector<sockaddr_storage> resolved_addrs;
  for (const auto& addr : addresses) {
    sockaddr_searcher searcher{addr, port, logger};
    for (struct addrinfo* result = searcher.next(); result != nullptr; result = searcher.next()) {
      sockaddr_storage storage = {};
      std::memcpy(&storage, result->ai_addr, result->ai_addrlen);
      resolved_addrs.emplace_back(storage);
    }
  }

  std::sort(resolved_addrs.begin(), resolved_addrs.end(), sockaddr_storage_less{});
  auto last = std::unique(resolved_addrs.begin(), resolved_addrs.end(), sockaddr_storage_equal);
  resolved_addrs.erase(last, resolved_addrs.end());
  return resolved_addrs;
}

static bool has_ipv6_address(const std::vector<sockaddr_storage>& addresses)
{
  return std::any_of(addresses.begin(), addresses.end(), [](const sockaddr_storage& addr) {
    return reinterpret_cast<const sockaddr*>(&addr)->sa_family == AF_INET6;
  });
}

static void keep_compatible_destinations(std::vector<sockaddr_storage>& destinations, int socket_family)
{
  if (socket_family != AF_INET) {
    return;
  }
  destinations.erase(std::remove_if(destinations.begin(),
                                    destinations.end(),
                                    [](const sockaddr_storage& addr) {
                                      return reinterpret_cast<const sockaddr*>(&addr)->sa_family == AF_INET6;
                                    }),
                     destinations.end());
}

static std::vector<std::string> get_peer_addresses(int fd, sctp_assoc_t assoc_id)
{
  std::vector<std::string> peer_addresses;
  struct sockaddr*        paddrs = nullptr;

  const int paddr_count = ::sctp_getpaddrs(fd, assoc_id, &paddrs);
  if (paddr_count <= 0 || paddrs == nullptr) {
    return peer_addresses;
  }

  const sockaddr* current = paddrs;
  for (int i = 0; i != paddr_count; ++i) {
    if (current->sa_family != AF_INET && current->sa_family != AF_INET6) {
      break;
    }
    const socklen_t peer_len = current->sa_family == AF_INET ? sizeof(sockaddr_in) : sizeof(sockaddr_in6);
    peer_addresses.push_back(sockaddr_to_string(*current, peer_len));
    current = reinterpret_cast<const sockaddr*>(reinterpret_cast<const uint8_t*>(current) + peer_len);
  }
  ::sctp_freepaddrs(paddrs);

  return peer_addresses;
}

static std::string get_local_endpoint(int fd)
{
  sockaddr_storage local_addr     = {};
  socklen_t        local_addr_len = sizeof(local_addr);
  if (::getsockname(fd, reinterpret_cast<sockaddr*>(&local_addr), &local_addr_len) != 0) {
    return "unknown";
  }
  return sockaddr_to_string(*reinterpret_cast<sockaddr*>(&local_addr), local_addr_len);
}

static void dump_hex(const byte_buffer& pdu)
{
  for (uint8_t byte : pdu) {
    std::printf("%02x", byte);
  }
  std::printf("\n");
}

static probe_config parse_args(int argc, char** argv)
{
  probe_config cfg;
  CLI::App     app{"Send one crafted NG Setup Request to a target AMF over SCTP"};

  app.add_option("--amf-addr,--target", cfg.amf_addresses, "Target AMF address or hostname")
      ->required()
      ->expected(1, -1);
  app.add_option("--amf-port,--port", cfg.amf_port, "Target AMF SCTP port")->capture_default_str();
  app.add_option("--bind-addr", cfg.bind_addresses, "Local SCTP bind address")->expected(1, -1);
  app.add_option("--bind-interface", cfg.bind_interface, "Local interface for SO_BINDTODEVICE")->capture_default_str();
  app.add_option("--sctp-init-max-attempts", cfg.init_max_attempts, "SCTP INIT max attempts")
      ->capture_default_str();
  app.add_option("--sctp-max-init-timeo-ms", cfg.max_init_timeo_ms, "SCTP INIT max timeout in milliseconds")
      ->capture_default_str();
  app.add_option("--post-send-wait-ms", cfg.post_send_wait_ms, "Delay after send before graceful close")
      ->capture_default_str();
  app.add_flag_function("--no-hex", [&cfg](std::int64_t) { cfg.dump_pdu_hex = false; },
                        "Do not print the packed NGAP PDU hex dump");

  try {
    app.parse(argc, argv);
  } catch (const CLI::ParseError& e) {
    std::exit(app.exit(e));
  }

  return cfg;
}

static int run_probe(const probe_config& cfg)
{
  auto& logger = ocudulog::fetch_basic_logger("SCTP-GW");

  const byte_buffer ng_setup_request = build_ng_setup_request();
  std::printf("NGAP probe: target=[%s]:%d bind=[%s] bind_interface=%s ppid=%u stream=%u\n",
              join_strings(cfg.amf_addresses, ", ").c_str(),
              cfg.amf_port,
              cfg.bind_addresses.empty() ? "implicit" : join_strings(cfg.bind_addresses, ", ").c_str(),
              cfg.bind_interface.c_str(),
              NGAP_PPID,
              stream_no);
  std::printf("NGAP probe: packed NGSetupRequest length=%zu bytes\n",
              static_cast<size_t>(ng_setup_request.length()));
  if (cfg.dump_pdu_hex) {
    std::printf("NGAP probe: packed NGSetupRequest hex=");
    dump_hex(ng_setup_request);
  }

  if (ng_setup_request.length() > network_gateway_sctp_mtu) {
    throw std::runtime_error("Packed NGSetupRequest exceeds SCTP gateway maximum PDU length");
  }

  std::vector<sockaddr_storage> dest_addrs = resolve_sctp_addresses(cfg.amf_addresses, cfg.amf_port, logger);
  if (dest_addrs.empty()) {
    throw std::runtime_error("Failed to resolve any target AMF address");
  }

  std::vector<sockaddr_storage> bind_addrs;
  if (!cfg.bind_addresses.empty()) {
    bind_addrs = resolve_sctp_addresses(cfg.bind_addresses, 0, logger);
    if (bind_addrs.empty()) {
      throw std::runtime_error("Failed to resolve any local bind address");
    }
  }

  const int socket_family = !bind_addrs.empty() ? (has_ipv6_address(bind_addrs) ? AF_INET6 : AF_INET)
                                                : (has_ipv6_address(dest_addrs) ? AF_INET6 : AF_INET);
  keep_compatible_destinations(dest_addrs, socket_family);
  if (dest_addrs.empty()) {
    throw std::runtime_error("No target AMF address is compatible with the selected SCTP socket family");
  }

  sctp_socket_params params = {};
  params.if_name           = "N2-PROBE";
  params.ai_family         = socket_family;
  params.ai_socktype       = SOCK_SEQPACKET;
  params.init_max_attempts = cfg.init_max_attempts;
  params.max_init_timeo    = std::chrono::milliseconds{cfg.max_init_timeo_ms};
  params.nodelay           = true;

  expected<sctp_socket> socket_outcome = sctp_socket::create(params);
  if (!socket_outcome.has_value()) {
    throw std::runtime_error("Failed to create SCTP socket");
  }
  sctp_socket socket = std::move(socket_outcome.value());

  if (!bind_addrs.empty() && !socket.bindx(bind_addrs, cfg.bind_interface)) {
    throw std::runtime_error("Failed to bind SCTP socket");
  }

  sctp_assoc_t assoc_id = 0;
  std::printf("NGAP probe: connecting to %zu resolved AMF address(es)...\n", dest_addrs.size());
  if (!socket.connectx(dest_addrs, assoc_id) || assoc_id == 0) {
    throw std::runtime_error(std::string("Failed to connect SCTP association: ") + std::strerror(errno));
  }

  const auto bound_port     = socket.get_bound_port();
  const auto peer_addresses = get_peer_addresses(socket.fd().value(), assoc_id);
  std::printf("NGAP probe: SCTP connected fd=%d assoc_id=%d local=%s local_port=%s peer=[%s]\n",
              socket.fd().value(),
              static_cast<int>(assoc_id),
              get_local_endpoint(socket.fd().value()).c_str(),
              bound_port.has_value() ? std::to_string(bound_port.value()).c_str() : "unknown",
              peer_addresses.empty() ? "unknown" : join_strings(peer_addresses, ", ").c_str());

  std::array<uint8_t, network_gateway_sctp_mtu> send_buffer = {};
  span<const uint8_t> ng_setup_request_span = to_span(ng_setup_request, send_buffer);
  const int bytes_sent = ::sctp_sendmsg(socket.fd().value(),
                                        ng_setup_request_span.data(),
                                        ng_setup_request_span.size(),
                                        reinterpret_cast<const sockaddr*>(&dest_addrs.front()),
                                        sockaddr_len(dest_addrs.front()),
                                        htonl(NGAP_PPID),
                                        0,
                                        stream_no,
                                        0,
                                        0);
  if (bytes_sent < 0) {
    throw std::runtime_error(std::string("Failed to send NGSetupRequest: ") + std::strerror(errno));
  }
  if (static_cast<size_t>(bytes_sent) != ng_setup_request.length()) {
    throw std::runtime_error("Partial NGSetupRequest send");
  }
  std::printf("NGAP probe: sent NGSetupRequest bytes=%d/%zu\n",
              bytes_sent,
              static_cast<size_t>(ng_setup_request.length()));

  if (cfg.post_send_wait_ms > 0) {
    std::this_thread::sleep_for(std::chrono::milliseconds{cfg.post_send_wait_ms});
  }

  const int eof_result =
      ::sctp_sendmsg(socket.fd().value(),
                     nullptr,
                     0,
                     reinterpret_cast<const sockaddr*>(&dest_addrs.front()),
                     sockaddr_len(dest_addrs.front()),
                     htonl(NGAP_PPID),
                     SCTP_EOF,
                     stream_no,
                     0,
                     0);
  if (eof_result < 0) {
    std::printf("NGAP probe: warning: failed to send SCTP EOF during close: %s\n", std::strerror(errno));
  } else {
    std::printf("NGAP probe: sent SCTP EOF and closing socket\n");
  }

  socket.close();
  return 0;
}

} // namespace

int main(int argc, char** argv)
{
  ocudulog::init();

  try {
    return run_probe(parse_args(argc, argv));
  } catch (const std::exception& e) {
    std::fprintf(stderr, "ngap_listener error: %s\n", e.what());
    return 1;
  }

  return 0;
}
