// UHM WINGS Fake Base Station Research

#pragma once

#include "config.h"
#include "ngap_decoder.h"
#include "state_machine.h"
#include "ocudu/adt/byte_buffer.h"
#include "ocudu/gateways/sctp_socket.h"
#include <memory>
#include <optional>
#include <sys/socket.h>
#include <netinet/sctp.h>
#include <vector>

namespace ocudu::fbs {

class n2_sctp_client
{
public:
  explicit n2_sctp_client(const injector_config& cfg_);
  ~n2_sctp_client();

  n2_sctp_client(const n2_sctp_client&)            = delete;
  n2_sctp_client& operator=(const n2_sctp_client&) = delete;

  void connect();
  void send_payload(const byte_buffer& payload);
  std::optional<byte_buffer> receive_payload(unsigned timeout_ms);
  void mark_ng_setup_accepted();
  void mark_ng_setup_failed();
  void close();

  ngap_connection_state state() const { return states.current(); }

private:
  const injector_config&       cfg;
  ngap_state_machine           states;
  std::unique_ptr<ocudu::sctp_socket> socket;
  sctp_assoc_t                 assoc_id = 0;
  std::vector<sockaddr_storage> destinations;

  [[noreturn]] void fail_with_state(const std::string& reason);
};

struct setup_exchange_result {
  ngap_connection_state state = ngap_connection_state::disconnected;
  ngap_message_summary  response_summary;
};

setup_exchange_result run_setup_exchange(const injector_config& cfg,
                                         const byte_buffer&      setup_request,
                                         unsigned               response_timeout_ms);

} // namespace ocudu::fbs
