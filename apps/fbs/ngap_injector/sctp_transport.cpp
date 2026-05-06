// UHM WINGS Fake Base Station Research

#include "sctp_transport.h"
#include "lib/gateways/sctp_network_gateway_common_impl.h"
#include "ocudu/adt/span.h"
#include "ocudu/gateways/sctp_network_gateway.h"
#include "ocudu/support/io/sockets.h"
#include <algorithm>
#include <array>
#include <arpa/inet.h>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <netdb.h>
#include <netinet/sctp.h>
#include <stdexcept>
#include <sys/socket.h>
#include <sys/time.h>

using namespace ocudu;
using namespace ocudu::fbs;

namespace {

static constexpr unsigned stream_no                 = 0;
static constexpr size_t   network_gateway_sctp_mtu = 9100;

socklen_t sockaddr_len(const sockaddr_storage& addr)
{
  return reinterpret_cast<const sockaddr*>(&addr)->sa_family == AF_INET ? sizeof(sockaddr_in) : sizeof(sockaddr_in6);
}

std::vector<sockaddr_storage>
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

bool has_ipv6_address(const std::vector<sockaddr_storage>& addresses)
{
  return std::any_of(addresses.begin(), addresses.end(), [](const sockaddr_storage& addr) {
    return reinterpret_cast<const sockaddr*>(&addr)->sa_family == AF_INET6;
  });
}

void keep_compatible_destinations(std::vector<sockaddr_storage>& destinations, int socket_family)
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

} // namespace

n2_sctp_client::n2_sctp_client(const injector_config& cfg_) : cfg(cfg_) {}

n2_sctp_client::~n2_sctp_client()
{
  close();
}

[[noreturn]] void n2_sctp_client::fail_with_state(const std::string& reason)
{
  if (states.can_transition_to(ngap_connection_state::ng_setup_failed)) {
    states.transition_to(ngap_connection_state::ng_setup_failed);
  }
  throw std::runtime_error(reason);
}

void n2_sctp_client::connect()
{
  auto& logger = ocudulog::fetch_basic_logger("NGAP-INJECTOR");

  destinations = resolve_sctp_addresses({cfg.amf_ip}, cfg.sctp_port, logger);
  if (destinations.empty()) {
    fail_with_state("Failed to resolve configured AMF IP");
  }

  std::vector<sockaddr_storage> bind_addrs = resolve_sctp_addresses({cfg.local_gnb_ip}, 0, logger);
  if (bind_addrs.empty()) {
    fail_with_state("Failed to resolve configured local gNB IP");
  }

  const int socket_family = has_ipv6_address(bind_addrs) ? AF_INET6 : AF_INET;
  keep_compatible_destinations(destinations, socket_family);
  if (destinations.empty()) {
    fail_with_state("No configured AMF address is compatible with the selected SCTP socket family");
  }

  sctp_socket_params params = {};
  params.if_name           = "N2-HARNESS";
  params.ai_family         = socket_family;
  params.ai_socktype       = SOCK_SEQPACKET;
  params.init_max_attempts = 2;
  params.max_init_timeo    = std::chrono::milliseconds{1000};
  params.nodelay           = true;

  expected<sctp_socket> socket_outcome = sctp_socket::create(params);
  if (!socket_outcome.has_value()) {
    fail_with_state("Failed to create SCTP socket");
  }
  socket = std::make_unique<sctp_socket>(std::move(socket_outcome.value()));

  if (!socket->bindx(bind_addrs, cfg.interface_name)) {
    fail_with_state("Failed to bind SCTP socket to configured local IP/interface");
  }

  if (!socket->connectx(destinations, assoc_id) || assoc_id == 0) {
    fail_with_state(std::string("Failed to connect SCTP association: ") + std::strerror(errno));
  }

  states.transition_to(ngap_connection_state::sctp_connected);
}

void n2_sctp_client::send_payload(const byte_buffer& payload)
{
  if (!socket) {
    fail_with_state("SCTP socket is not connected");
  }
  if (payload.length() > network_gateway_sctp_mtu) {
    fail_with_state("NGAP PDU exceeds SCTP MTU used by the harness");
  }

  const std::vector<uint8_t> bytes = to_vector(payload);
  const int bytes_sent = ::sctp_sendmsg(socket->fd().value(),
                                        bytes.data(),
                                        bytes.size(),
                                        reinterpret_cast<const sockaddr*>(&destinations.front()),
                                        sockaddr_len(destinations.front()),
                                        htonl(NGAP_PPID),
                                        0,
                                        stream_no,
                                        0,
                                        0);
  if (bytes_sent < 0) {
    fail_with_state(std::string("Failed to send NGAP PDU: ") + std::strerror(errno));
  }
  if (static_cast<size_t>(bytes_sent) != bytes.size()) {
    fail_with_state("Partial NGAP PDU send");
  }

  if (states.current() == ngap_connection_state::sctp_connected) {
    states.transition_to(ngap_connection_state::ng_setup_request_sent);
  }
}

void n2_sctp_client::mark_ng_setup_accepted()
{
  states.transition_to(ngap_connection_state::ng_setup_accepted);
  states.transition_to(ngap_connection_state::ready_for_test_messages);
}

void n2_sctp_client::mark_ng_setup_failed()
{
  states.transition_to(ngap_connection_state::ng_setup_failed);
}

std::optional<byte_buffer> n2_sctp_client::receive_payload(unsigned timeout_ms)
{
  if (!socket) {
    fail_with_state("SCTP socket is not connected");
  }

  timeval timeout = {};
  timeout.tv_sec  = static_cast<time_t>(timeout_ms / 1000U);
  timeout.tv_usec = static_cast<suseconds_t>((timeout_ms % 1000U) * 1000U);
  if (::setsockopt(socket->fd().value(), SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) != 0) {
    fail_with_state(std::string("Failed to set SCTP receive timeout: ") + std::strerror(errno));
  }

  std::array<uint8_t, network_gateway_sctp_mtu> rx_buffer = {};
  sockaddr_storage from = {};
  socklen_t        from_len = sizeof(from);
  sctp_sndrcvinfo  sri = {};
  int              flags = 0;

  const int bytes_read = ::sctp_recvmsg(socket->fd().value(),
                                        rx_buffer.data(),
                                        rx_buffer.size(),
                                        reinterpret_cast<sockaddr*>(&from),
                                        &from_len,
                                        &sri,
                                        &flags);
  if (bytes_read < 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      return std::nullopt;
    }
    fail_with_state(std::string("Failed to receive NGAP PDU: ") + std::strerror(errno));
  }
  if (bytes_read == 0 || ntohl(sri.sinfo_ppid) != NGAP_PPID) {
    return std::nullopt;
  }

  byte_buffer payload{byte_buffer::fallback_allocation_tag{}};
  if (!payload.append(span<const uint8_t>(rx_buffer.data(), static_cast<size_t>(bytes_read)))) {
    fail_with_state("Failed to allocate received NGAP PDU");
  }
  return payload;
}

void n2_sctp_client::close()
{
  if (socket) {
    socket->close();
    socket.reset();
  }
}

setup_exchange_result ocudu::fbs::run_setup_exchange(const injector_config& cfg,
                                                     const byte_buffer&      setup_request,
                                                     unsigned               response_timeout_ms)
{
  setup_exchange_result result = {};
  n2_sctp_client        client(cfg);

  client.connect();
  client.send_payload(setup_request);

  auto response = client.receive_payload(response_timeout_ms);
  if (!response) {
    client.mark_ng_setup_failed();
    result.state = ngap_connection_state::ng_setup_failed;
    result.response_summary.decode_ok = false;
    result.response_summary.decode_error = "Timed out waiting for AMF NG Setup response";
    return result;
  }

  result.response_summary = decode_ngap_payload(*response);
  if (is_ng_setup_response(result.response_summary)) {
    client.mark_ng_setup_accepted();
    result.state = ngap_connection_state::ready_for_test_messages;
  } else if (is_ng_setup_failure(result.response_summary)) {
    client.mark_ng_setup_failed();
    result.state = ngap_connection_state::ng_setup_failed;
  } else {
    client.mark_ng_setup_failed();
    result.state = ngap_connection_state::ng_setup_failed;
    if (result.response_summary.decode_ok) {
      result.response_summary.decode_error = "Received NGAP response, but it was not NG Setup Response/Failure";
    }
  }

  return result;
}
