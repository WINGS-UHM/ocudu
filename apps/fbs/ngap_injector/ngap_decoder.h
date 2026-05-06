// UHM WINGS Fake Base Station Research

#pragma once

#include "config.h"
#include "ocudu/adt/byte_buffer.h"
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace ocudu::fbs {

struct ngap_message_summary {
  bool                  decode_ok = false;
  std::string           decode_error;
  size_t                payload_length = 0;
  std::string           direction;
  std::string           message_type;
  uint16_t              procedure_code = 0;
  std::string           criticality;
  std::optional<uint64_t> ran_ue_ngap_id;
  std::optional<uint64_t> amf_ue_ngap_id;
  std::string           cause;
  std::string           guami;
  std::string           plmn;
  std::optional<uint64_t> tac;
  std::optional<uint64_t> gnb_id;
  std::vector<std::string> details;
};

ngap_message_summary decode_ngap_payload(const std::vector<uint8_t>& payload);
ngap_message_summary decode_ngap_payload(const byte_buffer& payload);

std::string format_summary(const ngap_message_summary& summary);

bool is_ng_setup_request(const ngap_message_summary& summary);
bool is_ng_setup_response(const ngap_message_summary& summary);
bool is_ng_setup_failure(const ngap_message_summary& summary);

byte_buffer build_ng_setup_request(const injector_config& cfg);
byte_buffer build_ue_context_release_request(uint64_t amf_ue_ngap_id, uint64_t ran_ue_ngap_id);
byte_buffer build_ng_reset_message();

std::vector<uint8_t> to_vector(const byte_buffer& payload);

} // namespace ocudu::fbs
