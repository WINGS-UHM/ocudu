// UHM WINGS Fake Base Station Research

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace ocudu::fbs {

struct ngap_pcap_packet {
  unsigned             packet_index = 0;
  unsigned             ngap_index   = 0;
  uint64_t             timestamp_us = 0;
  std::string          source_ip;
  std::string          destination_ip;
  uint16_t             source_port      = 0;
  uint16_t             destination_port = 0;
  uint32_t             ppid             = 0;
  std::vector<uint8_t> payload;
};

std::vector<ngap_pcap_packet> read_ngap_packets_from_pcap(const std::string& path);

std::vector<ngap_pcap_packet>
extract_ngap_payloads_from_packet(const uint8_t* data, size_t length, unsigned link_type, unsigned packet_index = 0);

bool packet_endpoints_are_configured_lab_ips(const ngap_pcap_packet& packet,
                                             const std::string&      local_ip,
                                             const std::string&      amf_ip,
                                             const std::vector<std::string>& allowlisted_amf_ips);

} // namespace ocudu::fbs
