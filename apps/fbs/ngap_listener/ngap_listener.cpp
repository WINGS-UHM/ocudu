// UHM WINGS Fake Base Station Research

#include "CLI/CLI11.hpp"
#include "lib/gateways/sctp_network_gateway_common_impl.h"
#include "ocudu/asn1/asn1_utils.h"
#include "ocudu/asn1/ngap/common.h"
#include "ocudu/asn1/ngap/ngap.h"
#include "ocudu/gateways/sctp_network_gateway.h"
#include "ocudu/gateways/sctp_socket.h"
#include "ocudu/ngap/ngap_message.h"
#include "ocudu/ocudulog/ocudulog.h"
#include "ocudu/support/io/sockets.h"
#include <algorithm>
#include <array>
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <deque>
#include <iostream>
#include <map>
#include <net/if.h>
#include <netdb.h>
#include <netinet/sctp.h>
#include <optional>
#include <poll.h>
#include <regex>
#include <set>
#include <stdexcept>
#include <string>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

using namespace ocudu;
using namespace ocudu::ocucp;
using namespace asn1::ngap;

namespace {

static constexpr size_t network_gateway_sctp_mtu = 9100;

struct listener_config {
  std::vector<std::string> listen_addresses = {"0.0.0.0"};
  int                      listen_port      = NGAP_PORT;
  std::string              bind_interface   = "auto";
  bool                     dump_pdu_hex     = true;
  bool                     promiscuous_mode = false;
};

struct message_context {
  std::vector<std::string> amf_ue_ngap_ids;
  std::vector<std::string> ran_ue_ngap_ids;
  std::vector<std::string> pdu_session_ids;
  std::vector<std::string> qfis;
  std::vector<std::string> teids;
  std::vector<std::string> transport_layer_addresses;
  std::vector<std::string> nas_pdus;
  std::vector<std::string> plmns;
  std::vector<std::string> tacs;
};

struct observed_message {
  size_t          sequence = 0;
  std::string     source;
  int             assoc_id = 0;
  unsigned        stream   = 0;
  unsigned        ppid     = 0;
  size_t          length   = 0;
  std::string     ngap_type;
  bool            decoded = false;
  message_context context;
  std::string     json;
};

class context_queue
{
public:
  void push(observed_message message)
  {
    message.sequence = next_sequence++;
    add_context(message.context);
    messages.push_back(std::move(message));
    print_summary(messages.back());
  }

private:
  static void insert_all(std::set<std::string>& target, const std::vector<std::string>& values)
  {
    for (const auto& value : values) {
      if (!value.empty()) {
        target.insert(value);
      }
    }
  }

  static void print_values(const char* label, const std::set<std::string>& values)
  {
    std::printf("  %s: [", label);
    bool first = true;
    for (const auto& value : values) {
      std::printf("%s%s", first ? "" : ", ", value.c_str());
      first = false;
    }
    std::printf("]\n");
  }

  static void print_latest_values(const char* label, const std::vector<std::string>& values)
  {
    std::printf("  %s: [", label);
    for (size_t i = 0; i != values.size(); ++i) {
      std::printf("%s%s", i == 0 ? "" : ", ", values[i].c_str());
    }
    std::printf("]\n");
  }

  void add_context(const message_context& context)
  {
    insert_all(amf_ue_ngap_ids, context.amf_ue_ngap_ids);
    insert_all(ran_ue_ngap_ids, context.ran_ue_ngap_ids);
    insert_all(pdu_session_ids, context.pdu_session_ids);
    insert_all(qfis, context.qfis);
    insert_all(teids, context.teids);
    insert_all(transport_layer_addresses, context.transport_layer_addresses);
    insert_all(nas_pdus, context.nas_pdus);
    insert_all(plmns, context.plmns);
    insert_all(tacs, context.tacs);
  }

  void print_summary(const observed_message& message) const
  {
    std::printf("NGAP listener: queued message #%zu type=%s decoded=%s source=%s assoc_id=%d stream=%u ppid=%u length=%zu\n",
                message.sequence,
                message.ngap_type.empty() ? "unknown" : message.ngap_type.c_str(),
                message.decoded ? "yes" : "no",
                message.source.c_str(),
                message.assoc_id,
                message.stream,
                message.ppid,
                message.length);
    std::printf("NGAP listener: extracted context for latest message:\n");
    print_latest_values("AMF-UE-NGAP-ID", message.context.amf_ue_ngap_ids);
    print_latest_values("RAN-UE-NGAP-ID", message.context.ran_ue_ngap_ids);
    print_latest_values("PDU-Session-ID", message.context.pdu_session_ids);
    print_latest_values("QFI", message.context.qfis);
    print_latest_values("TEID", message.context.teids);
    print_latest_values("TransportLayerAddress", message.context.transport_layer_addresses);
    print_latest_values("NAS-PDU", message.context.nas_pdus);
    print_latest_values("PLMN", message.context.plmns);
    print_latest_values("TAC", message.context.tacs);
    std::printf("NGAP listener: accumulated context lists, queue_depth=%zu:\n", messages.size());
    print_values("AMF-UE-NGAP-ID", amf_ue_ngap_ids);
    print_values("RAN-UE-NGAP-ID", ran_ue_ngap_ids);
    print_values("PDU-Session-ID", pdu_session_ids);
    print_values("QFI", qfis);
    print_values("TEID", teids);
    print_values("TransportLayerAddress", transport_layer_addresses);
    print_values("NAS-PDU", nas_pdus);
    print_values("PLMN", plmns);
    print_values("TAC", tacs);
  }

  size_t                       next_sequence = 1;
  std::deque<observed_message> messages;
  std::set<std::string>        amf_ue_ngap_ids;
  std::set<std::string>        ran_ue_ngap_ids;
  std::set<std::string>        pdu_session_ids;
  std::set<std::string>        qfis;
  std::set<std::string>        teids;
  std::set<std::string>        transport_layer_addresses;
  std::set<std::string>        nas_pdus;
  std::set<std::string>        plmns;
  std::set<std::string>        tacs;
};

class promiscuous_mode_guard
{
public:
  promiscuous_mode_guard() = default;

  explicit promiscuous_mode_guard(const std::string& if_name) { enable(if_name); }

  ~promiscuous_mode_guard() { reset(); }

  promiscuous_mode_guard(const promiscuous_mode_guard&)            = delete;
  promiscuous_mode_guard& operator=(const promiscuous_mode_guard&) = delete;

private:
  void enable(const std::string& if_name)
  {
    if (if_name.empty() || if_name == "auto") {
      throw std::runtime_error("Promiscuous mode requires an explicit --bind-interface");
    }
    if (if_name.size() >= IFNAMSIZ) {
      throw std::runtime_error("Interface name is too long for promiscuous mode");
    }

    ctl_fd = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (ctl_fd < 0) {
      throw std::runtime_error(std::string("Failed to create interface control socket: ") + std::strerror(errno));
    }

    ifreq ifr = {};
    std::strncpy(ifr.ifr_name, if_name.c_str(), IFNAMSIZ - 1);
    if (::ioctl(ctl_fd, SIOCGIFFLAGS, &ifr) < 0) {
      const std::string error = std::strerror(errno);
      ::close(ctl_fd);
      ctl_fd = -1;
      throw std::runtime_error("Failed to read interface flags for " + if_name + ": " + error);
    }

    interface_name = if_name;
    previous_flags = ifr.ifr_flags;
    active         = true;

    if ((ifr.ifr_flags & IFF_PROMISC) != 0) {
      std::printf("NGAP listener: interface %s is already in promiscuous mode\n", interface_name.c_str());
      return;
    }

    ifr.ifr_flags = static_cast<short>(ifr.ifr_flags | IFF_PROMISC);
    if (::ioctl(ctl_fd, SIOCSIFFLAGS, &ifr) < 0) {
      const std::string error = std::strerror(errno);
      reset();
      throw std::runtime_error("Failed to enable promiscuous mode for " + if_name + ": " + error);
    }

    changed = true;
    std::printf("NGAP listener: enabled promiscuous mode on interface %s\n", interface_name.c_str());
  }

  void reset()
  {
    if (!active) {
      return;
    }

    if (changed && ctl_fd >= 0) {
      ifreq ifr = {};
      std::strncpy(ifr.ifr_name, interface_name.c_str(), IFNAMSIZ - 1);
      ifr.ifr_flags = previous_flags;
      if (::ioctl(ctl_fd, SIOCSIFFLAGS, &ifr) < 0) {
        std::printf("NGAP listener: warning: failed to restore interface flags for %s: %s\n",
                    interface_name.c_str(),
                    std::strerror(errno));
      } else {
        std::printf("NGAP listener: restored interface flags for %s\n", interface_name.c_str());
      }
    }

    if (ctl_fd >= 0) {
      ::close(ctl_fd);
      ctl_fd = -1;
    }
    active = false;
  }

  std::string interface_name;
  short       previous_flags = 0;
  int         ctl_fd         = -1;
  bool        active         = false;
  bool        changed        = false;
};

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

static std::string escape_regex(const std::string& text)
{
  static const std::regex special{R"([.^$|()\[\]{}*+?\\])"};
  return std::regex_replace(text, special, R"(\$&)" );
}

static void add_unique(std::vector<std::string>& values, const std::string& value)
{
  if (!value.empty() && std::find(values.begin(), values.end(), value) == values.end()) {
    values.push_back(value);
  }
}

static std::vector<std::string> extract_json_key_values(const std::string& json, const std::string& key)
{
  std::vector<std::string> values;
  const std::regex pattern{"\\\"" + escape_regex(key) + "\\\"\\s*:\\s*(?:\\\"([^\\\"]*)\\\"|([0-9]+))"};
  for (std::sregex_iterator it(json.begin(), json.end(), pattern), end; it != end; ++it) {
    add_unique(values, (*it)[1].matched ? (*it)[1].str() : (*it)[2].str());
  }
  return values;
}

static std::vector<std::string> extract_ie_values(const std::string& json, const std::string& ie_name)
{
  std::vector<std::string> values;
  const std::regex pattern{"\\\"id\\\"\\s*:\\s*\\\"" + escape_regex(ie_name) +
                           "\\\"[\\s\\S]*?\\\"Value\\\"\\s*:\\s*(?:\\\"([^\\\"]*)\\\"|([0-9]+))"};
  for (std::sregex_iterator it(json.begin(), json.end(), pattern), end; it != end; ++it) {
    add_unique(values, (*it)[1].matched ? (*it)[1].str() : (*it)[2].str());
  }
  return values;
}

static void append_values(std::vector<std::string>& target, const std::vector<std::string>& source)
{
  for (const auto& value : source) {
    add_unique(target, value);
  }
}

static message_context extract_context(const std::string& json)
{
  message_context context;

  append_values(context.amf_ue_ngap_ids, extract_ie_values(json, "AMF-UE-NGAP-ID"));
  append_values(context.ran_ue_ngap_ids, extract_ie_values(json, "RAN-UE-NGAP-ID"));
  append_values(context.pdu_session_ids, extract_ie_values(json, "PDU-Session-ID"));
  append_values(context.nas_pdus, extract_ie_values(json, "NAS-PDU"));

  append_values(context.pdu_session_ids, extract_json_key_values(json, "pDUSessionID"));
  append_values(context.qfis, extract_json_key_values(json, "qosFlowIdentifier"));
  append_values(context.teids, extract_json_key_values(json, "gTP-TEID"));
  append_values(context.transport_layer_addresses, extract_json_key_values(json, "transportLayerAddress"));
  append_values(context.nas_pdus, extract_json_key_values(json, "nAS-PDU"));
  append_values(context.nas_pdus, extract_json_key_values(json, "pDUSessionNAS-PDU"));
  append_values(context.plmns, extract_json_key_values(json, "pLMNIdentity"));
  append_values(context.plmns, extract_json_key_values(json, "pLMN-ID"));
  append_values(context.tacs, extract_json_key_values(json, "tAC"));

  return context;
}

static std::string get_message_type(const ngap_pdu_c& pdu)
{
  switch (pdu.type().value) {
    case ngap_pdu_c::types_opts::init_msg:
      return pdu.init_msg().value.type().to_string();
    case ngap_pdu_c::types_opts::successful_outcome:
      return pdu.successful_outcome().value.type().to_string();
    case ngap_pdu_c::types_opts::unsuccessful_outcome:
      return pdu.unsuccessful_outcome().value.type().to_string();
    default:
      return "unknown";
  }
}

static observed_message decode_ngap_pdu(const byte_buffer& pdu,
                                        const std::string& source,
                                        int                assoc_id,
                                        unsigned           stream,
                                        unsigned           ppid,
                                        bool               dump_pdu_hex)
{
  observed_message observed;
  observed.source   = source;
  observed.assoc_id = assoc_id;
  observed.stream   = stream;
  observed.ppid     = ppid;
  observed.length   = pdu.length();

  std::printf("NGAP listener: received NGAP PDU length=%zu bytes\n", static_cast<size_t>(pdu.length()));
  if (dump_pdu_hex) {
    std::printf("NGAP listener: received NGAP PDU hex=");
    dump_hex(pdu);
  }

  asn1::cbit_ref bref(pdu);
  ngap_message   msg = {};
  if (msg.pdu.unpack(bref) != asn1::OCUDUASN_SUCCESS) {
    std::printf("NGAP listener: failed to decode NGAP PDU\n");
    observed.ngap_type = "decode-failed";
    return observed;
  }

  asn1::json_writer json;
  msg.pdu.to_json(json);
  observed.decoded   = true;
  observed.ngap_type = get_message_type(msg.pdu);
  observed.json      = json.to_string();
  observed.context   = extract_context(observed.json);

  std::printf("NGAP listener: decoded NGAP PDU JSON:\n%s\n", observed.json.c_str());
  return observed;
}

static void receive_ngap_loop(sctp_socket& socket, bool dump_pdu_hex)
{
  std::printf("NGAP listener: passively listening for NGAP packets. Enter q and press return to quit.\n");

  context_queue                                      queue;
  std::array<uint8_t, network_gateway_sctp_mtu> recv_buffer = {};
  while (true) {
    std::array<pollfd, 2> fds = {};
    fds[0].fd                 = socket.fd().value();
    fds[0].events             = POLLIN;
    fds[1].fd                 = STDIN_FILENO;
    fds[1].events             = POLLIN;

    const int poll_result = ::poll(fds.data(), fds.size(), -1);
    if (poll_result < 0) {
      if (errno == EINTR) {
        continue;
      }
      throw std::runtime_error(std::string("poll failed: ") + std::strerror(errno));
    }

    if ((fds[1].revents & POLLIN) != 0) {
      std::string input;
      if (!std::getline(std::cin, input) || input == "q" || input == "quit" || input == "exit") {
        return;
      }
      std::printf("NGAP listener: enter q to quit; continuing to listen\n");
    }

    if ((fds[0].revents & (POLLERR | POLLHUP | POLLNVAL)) != 0) {
      throw std::runtime_error("SCTP socket reported an error or hangup");
    }

    if ((fds[0].revents & POLLIN) == 0) {
      continue;
    }

    struct sctp_sndrcvinfo sri       = {};
    int                    msg_flags = 0;
    sockaddr_storage       msg_src   = {};
    socklen_t              msg_len   = sizeof(msg_src);

    const int rx_bytes = ::sctp_recvmsg(socket.fd().value(),
                                        recv_buffer.data(),
                                        recv_buffer.size(),
                                        reinterpret_cast<sockaddr*>(&msg_src),
                                        &msg_len,
                                        &sri,
                                        &msg_flags);
    if (rx_bytes < 0) {
      if (errno == EAGAIN || errno == EINTR) {
        continue;
      }
      throw std::runtime_error(std::string("Error reading from SCTP socket: ") + std::strerror(errno));
    }
    if (rx_bytes == 0) {
      std::printf("NGAP listener: SCTP peer closed an association\n");
      continue;
    }

    if ((msg_flags & MSG_NOTIFICATION) != 0) {
      std::printf("NGAP listener: received SCTP notification length=%d bytes\n", rx_bytes);
      continue;
    }

    const std::string source = sockaddr_to_string(*reinterpret_cast<sockaddr*>(&msg_src), msg_len);
    std::printf("NGAP listener: packet source=%s assoc_id=%d stream=%u ppid=%u\n",
                source.c_str(),
                static_cast<int>(sri.sinfo_assoc_id),
                sri.sinfo_stream,
                ntohl(sri.sinfo_ppid));

    byte_buffer pdu{byte_buffer::fallback_allocation_tag{},
                    span<const uint8_t>(recv_buffer.data(), static_cast<size_t>(rx_bytes))};
    queue.push(decode_ngap_pdu(pdu,
                               source,
                               static_cast<int>(sri.sinfo_assoc_id),
                               sri.sinfo_stream,
                               ntohl(sri.sinfo_ppid),
                               dump_pdu_hex));
  }
}

static listener_config parse_args(int argc, char** argv)
{
  listener_config cfg;
  CLI::App        app{"Passively listen for SCTP/NGAP packets and collect reusable NGAP/NAS context"};

  app.add_option("--listen-addr,--bind-addr", cfg.listen_addresses, "Local SCTP listen address")
      ->expected(1, -1)
      ->capture_default_str();
  app.add_option("--listen-port,--port", cfg.listen_port, "Local SCTP listen port")->capture_default_str();
  app.add_option("--bind-interface", cfg.bind_interface, "Local interface for SO_BINDTODEVICE")->capture_default_str();
  app.add_flag("--promiscuous",
               cfg.promiscuous_mode,
               "Enable promiscuous mode on --bind-interface while the listener is running");
  app.add_flag_function("--no-hex", [&cfg](std::int64_t) { cfg.dump_pdu_hex = false; }, "Do not print NGAP PDU hex dumps");

  try {
    app.parse(argc, argv);
  } catch (const CLI::ParseError& e) {
    std::exit(app.exit(e));
  }

  return cfg;
}

static int run_listener(const listener_config& cfg)
{
  auto& logger = ocudulog::fetch_basic_logger("SCTP-GW");

  std::printf("NGAP listener: listen=[%s]:%d bind_interface=%s ppid=%u\n",
              join_strings(cfg.listen_addresses, ", ").c_str(),
              cfg.listen_port,
              cfg.bind_interface.c_str(),
              NGAP_PPID);

  std::optional<promiscuous_mode_guard> promisc_guard;
  if (cfg.promiscuous_mode) {
    promisc_guard.emplace(cfg.bind_interface);
  }

  std::vector<sockaddr_storage> bind_addrs = resolve_sctp_addresses(cfg.listen_addresses, cfg.listen_port, logger);
  if (bind_addrs.empty()) {
    throw std::runtime_error("Failed to resolve any local listen address");
  }

  const int socket_family = has_ipv6_address(bind_addrs) ? AF_INET6 : AF_INET;

  sctp_socket_params params = {};
  params.if_name           = "N2-LISTENER";
  params.ai_family         = socket_family;
  params.ai_socktype       = SOCK_SEQPACKET;
  params.reuse_addr        = true;
  params.nodelay           = true;

  expected<sctp_socket> socket_outcome = sctp_socket::create(params);
  if (!socket_outcome.has_value()) {
    throw std::runtime_error("Failed to create SCTP socket");
  }
  sctp_socket socket = std::move(socket_outcome.value());

  if (!socket.bindx(bind_addrs, cfg.bind_interface)) {
    throw std::runtime_error("Failed to bind SCTP socket");
  }
  if (!socket.listen()) {
    throw std::runtime_error("Failed to listen on SCTP socket");
  }

  const auto bound_port = socket.get_bound_port();
  std::printf("NGAP listener: SCTP passive socket fd=%d local=%s local_port=%s\n",
              socket.fd().value(),
              get_local_endpoint(socket.fd().value()).c_str(),
              bound_port.has_value() ? std::to_string(bound_port.value()).c_str() : "unknown");

  receive_ngap_loop(socket, cfg.dump_pdu_hex);

  socket.close();
  return 0;
}

} // namespace

int main(int argc, char** argv)
{
  ocudulog::init();

  try {
    return run_listener(parse_args(argc, argv));
  } catch (const std::exception& e) {
    std::fprintf(stderr, "ngap_listener error: %s\n", e.what());
    return 1;
  }

  return 0;
}
