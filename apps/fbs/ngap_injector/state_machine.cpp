// UHM WINGS Fake Base Station Research

#include "state_machine.h"
#include <cstdio>
#include <stdexcept>

using namespace ocudu::fbs;

bool ngap_state_machine::can_transition_to(ngap_connection_state next) const
{
  switch (state) {
    case ngap_connection_state::disconnected:
      return next == ngap_connection_state::sctp_connected || next == ngap_connection_state::ng_setup_failed;
    case ngap_connection_state::sctp_connected:
      return next == ngap_connection_state::ng_setup_request_sent || next == ngap_connection_state::ng_setup_failed;
    case ngap_connection_state::ng_setup_request_sent:
      return next == ngap_connection_state::ng_setup_accepted || next == ngap_connection_state::ng_setup_failed;
    case ngap_connection_state::ng_setup_accepted:
      return next == ngap_connection_state::ready_for_test_messages || next == ngap_connection_state::ng_setup_failed;
    case ngap_connection_state::ready_for_test_messages:
      return next == ngap_connection_state::ng_setup_failed || next == ngap_connection_state::disconnected;
    case ngap_connection_state::ng_setup_failed:
      return next == ngap_connection_state::disconnected;
  }
  return false;
}

void ngap_state_machine::transition_to(ngap_connection_state next)
{
  if (!can_transition_to(next)) {
    throw std::runtime_error(std::string("Invalid NGAP state transition from ") + to_string(state) + " to " +
                             to_string(next));
  }
  std::printf("state_transition from=%s to=%s\n", to_string(state), to_string(next));
  state = next;
}
