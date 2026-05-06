// UHM WINGS Fake Base Station Research

#pragma once

#include "types.h"

namespace ocudu::fbs {

class ngap_state_machine
{
public:
  ngap_connection_state current() const { return state; }

  bool can_transition_to(ngap_connection_state next) const;
  void transition_to(ngap_connection_state next);

private:
  ngap_connection_state state = ngap_connection_state::disconnected;
};

} // namespace ocudu::fbs
