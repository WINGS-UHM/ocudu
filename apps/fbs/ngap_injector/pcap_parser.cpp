// UHM WINGS Fake Base Station Research

#include "pcap_parser.h"
#include "ocudu/gateways/sctp_network_gateway.h"
#include <algorithm>
#include <arpa/inet.h>
#include <cstring>
#include <fstream>
#include <netinet/in.h>
#include <stdexcept>

using namespace ocudu::fbs;

namespace {

constexpr unsigned dlt_en10mb    = 1;
constexpr unsigned dlt_raw       = 101;
constexpr unsigned dlt_linux_sll = 113;
constexpr unsigned dlt_linux_sll2 = 276;

uint16_t read_be16(const uint8_t* p)
{
  return static_cast<uint16_t>((static_cast<uint16_t>(p[0]) << 8U) | p[1]);
}

uint32_t read_be32(const uint8_t* p)
{
  return (static_cast<uint32_t>(p[0]) << 24U) | (static_cast<uint32_t>(p[1]) << 16U) |
         (static_cast<uint32_t>(p[2]) << 8U) | p[3];
}

uint32_t read_u32(const uint8_t* p, bool little)
{
  if (little) {
    return (static_cast<uint32_t>(p[3]) << 24U) | (static_cast<uint32_t>(p[2]) << 16U) |
           (static_cast<uint32_t>(p[1]) << 8U) | p[0];
  }
  return read_be32(p);
}

uint16_t read_u16(const uint8_t* p, bool little)
{
  if (little) {
    return static_cast<uint16_t>((static_cast<uint16_t>(p[1]) << 8U) | p[0]);
  }
  return read_be16(p);
}

std::vector<uint8_t> read_file(const std::string& path)
{
  std::ifstream input(path, std::ios::binary);
  if (!input) {
    throw std::runtime_error("Failed to open PCAP file: " + path);
  }
  input.seekg(0, std::ios::end);
  const auto size = input.tellg();
  input.seekg(0, std::ios::beg);
  std::vector<uint8_t> bytes(static_cast<size_t>(size));
  if (!bytes.empty()) {
    input.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
  }
  return bytes;
}

std::string ipv4_to_string(const uint8_t* p)
{
  char out[INET_ADDRSTRLEN] = {};
  ::inet_ntop(AF_INET, p, out, sizeof(out));
  return out;
}

std::string ipv6_to_string(const uint8_t* p)
{
  char out[INET6_ADDRSTRLEN] = {};
  ::inet_ntop(AF_INET6, p, out, sizeof(out));
  return out;
}

size_t align32(size_t value)
{
  return (value + 3U) & ~static_cast<size_t>(3U);
}

void extract_sctp_data_chunks(const uint8_t*   sctp,
                              size_t           length,
                              ngap_pcap_packet base,
                              std::vector<ngap_pcap_packet>& out)
{
  if (length < 12) {
    return;
  }

  base.source_port      = read_be16(sctp);
  base.destination_port = read_be16(sctp + 2);

  size_t offset = 12;
  while (offset + 4 <= length) {
    const uint8_t  chunk_type = sctp[offset];
    const uint8_t  flags      = sctp[offset + 1];
    const uint16_t chunk_len  = read_be16(sctp + offset + 2);
    if (chunk_len < 4 || offset + chunk_len > length) {
      return;
    }

    if (chunk_type == 0 && chunk_len >= 16) {
      const bool is_complete_user_msg = (flags & 0x03U) == 0x03U;
      const uint32_t ppid             = read_be32(sctp + offset + 12);
      if (is_complete_user_msg && ppid == ocudu::NGAP_PPID) {
        ngap_pcap_packet packet = base;
        packet.ppid             = ppid;
        packet.payload.assign(sctp + offset + 16, sctp + offset + chunk_len);
        packet.ngap_index = static_cast<unsigned>(out.size());
        out.push_back(std::move(packet));
      }
    }
    offset += align32(chunk_len);
  }
}

void extract_from_ip_packet(const uint8_t* data,
                            size_t        length,
                            ngap_pcap_packet base,
                            std::vector<ngap_pcap_packet>& out)
{
  if (length < 1) {
    return;
  }
  const uint8_t version = data[0] >> 4U;

  if (version == 4) {
    if (length < 20) {
      return;
    }
    const size_t ihl = static_cast<size_t>(data[0] & 0x0fU) * 4U;
    if (ihl < 20 || length < ihl) {
      return;
    }
    const uint16_t flags_offset = read_be16(data + 6);
    if ((flags_offset & 0x3fffU) != 0) {
      return;
    }
    if (data[9] != IPPROTO_SCTP) {
      return;
    }
    const uint16_t total_len = read_be16(data + 2);
    if (total_len < ihl || total_len > length) {
      return;
    }
    base.source_ip      = ipv4_to_string(data + 12);
    base.destination_ip = ipv4_to_string(data + 16);
    extract_sctp_data_chunks(data + ihl, total_len - ihl, std::move(base), out);
    return;
  }

  if (version == 6) {
    if (length < 40) {
      return;
    }
    uint8_t next_header = data[6];
    size_t  offset      = 40;
    while (next_header == 0 || next_header == 43 || next_header == 60 || next_header == 51) {
      if (offset + 2 > length) {
        return;
      }
      const uint8_t following = data[offset];
      const size_t  ext_len   = next_header == 51 ? (static_cast<size_t>(data[offset + 1]) + 2U) * 4U
                                                   : (static_cast<size_t>(data[offset + 1]) + 1U) * 8U;
      if (offset + ext_len > length) {
        return;
      }
      next_header = following;
      offset += ext_len;
    }
    if (next_header == 44) {
      return;
    }
    if (next_header != IPPROTO_SCTP) {
      return;
    }
    base.source_ip      = ipv6_to_string(data + 8);
    base.destination_ip = ipv6_to_string(data + 24);
    extract_sctp_data_chunks(data + offset, length - offset, std::move(base), out);
  }
}

void extract_frame_payload(const uint8_t* data,
                           size_t        length,
                           unsigned      link_type,
                           ngap_pcap_packet base,
                           std::vector<ngap_pcap_packet>& out)
{
  if (link_type == dlt_en10mb) {
    if (length < 14) {
      return;
    }
    size_t   offset    = 14;
    uint16_t etherType = read_be16(data + 12);
    while ((etherType == 0x8100 || etherType == 0x88a8) && offset + 4 <= length) {
      etherType = read_be16(data + offset + 2);
      offset += 4;
    }
    if (etherType == 0x0800 || etherType == 0x86dd) {
      extract_from_ip_packet(data + offset, length - offset, std::move(base), out);
    }
    return;
  }

  if (link_type == dlt_raw) {
    extract_from_ip_packet(data, length, std::move(base), out);
    return;
  }

  if (link_type == dlt_linux_sll) {
    if (length < 16) {
      return;
    }
    const uint16_t proto = read_be16(data + 14);
    if (proto == 0x0800 || proto == 0x86dd) {
      extract_from_ip_packet(data + 16, length - 16, std::move(base), out);
    }
    return;
  }

  if (link_type == dlt_linux_sll2) {
    if (length < 20) {
      return;
    }
    const uint16_t proto = read_be16(data);
    if (proto == 0x0800 || proto == 0x86dd) {
      extract_from_ip_packet(data + 20, length - 20, std::move(base), out);
    }
  }
}

bool is_pcapng(const std::vector<uint8_t>& bytes)
{
  return bytes.size() >= 12 && bytes[0] == 0x0a && bytes[1] == 0x0d && bytes[2] == 0x0d && bytes[3] == 0x0a;
}

std::vector<ngap_pcap_packet> parse_pcap(const std::vector<uint8_t>& bytes)
{
  if (bytes.size() < 24) {
    throw std::runtime_error("PCAP file is too short");
  }

  bool little = false;
  if (bytes[0] == 0xd4 && bytes[1] == 0xc3 && bytes[2] == 0xb2 && bytes[3] == 0xa1) {
    little = true;
  } else if (bytes[0] == 0xa1 && bytes[1] == 0xb2 && bytes[2] == 0xc3 && bytes[3] == 0xd4) {
    little = false;
  } else if (bytes[0] == 0x4d && bytes[1] == 0x3c && bytes[2] == 0xb2 && bytes[3] == 0xa1) {
    little = true;
  } else if (bytes[0] == 0xa1 && bytes[1] == 0xb2 && bytes[2] == 0x3c && bytes[3] == 0x4d) {
    little = false;
  } else {
    throw std::runtime_error("Unsupported PCAP magic");
  }

  const unsigned link_type = read_u32(bytes.data() + 20, little);
  size_t         offset    = 24;
  unsigned       packet_index = 0;
  std::vector<ngap_pcap_packet> out;
  while (offset + 16 <= bytes.size()) {
    const uint32_t ts_sec   = read_u32(bytes.data() + offset, little);
    const uint32_t ts_usec  = read_u32(bytes.data() + offset + 4, little);
    const uint32_t incl_len = read_u32(bytes.data() + offset + 8, little);
    offset += 16;
    if (offset + incl_len > bytes.size()) {
      break;
    }
    ngap_pcap_packet base = {};
    base.packet_index     = packet_index++;
    base.timestamp_us     = static_cast<uint64_t>(ts_sec) * 1000000ULL + ts_usec;
    extract_frame_payload(bytes.data() + offset, incl_len, link_type, std::move(base), out);
    offset += incl_len;
  }
  for (unsigned i = 0; i != out.size(); ++i) {
    out[i].ngap_index = i;
  }
  return out;
}

std::vector<ngap_pcap_packet> parse_pcapng(const std::vector<uint8_t>& bytes)
{
  size_t offset = 0;
  bool   little = true;
  std::vector<unsigned> interface_link_types;
  std::vector<ngap_pcap_packet> out;

  while (offset + 12 <= bytes.size()) {
    const uint32_t block_type = read_u32(bytes.data() + offset, little);
    const uint32_t block_len  = read_u32(bytes.data() + offset + 4, little);
    if (block_len < 12 || offset + block_len > bytes.size()) {
      break;
    }
    const uint8_t* block = bytes.data() + offset;

    if (block_type == 0x0a0d0d0aU) {
      if (block_len >= 16) {
        const uint32_t bom_le = read_u32(block + 8, true);
        const uint32_t bom_be = read_u32(block + 8, false);
        if (bom_le == 0x1a2b3c4dU) {
          little = true;
        } else if (bom_be == 0x1a2b3c4dU) {
          little = false;
        }
      }
    } else if (block_type == 0x00000001U) {
      if (block_len >= 20) {
        interface_link_types.push_back(read_u16(block + 8, little));
      }
    } else if (block_type == 0x00000006U) {
      if (block_len >= 32) {
        const uint32_t interface_id = read_u32(block + 8, little);
        const uint32_t ts_high      = read_u32(block + 12, little);
        const uint32_t ts_low       = read_u32(block + 16, little);
        const uint32_t captured_len = read_u32(block + 20, little);
        if (interface_id < interface_link_types.size() && 28ULL + captured_len <= block_len) {
          ngap_pcap_packet base = {};
          base.packet_index     = static_cast<unsigned>(out.size());
          base.timestamp_us     = (static_cast<uint64_t>(ts_high) << 32U) | ts_low;
          extract_frame_payload(block + 28, captured_len, interface_link_types[interface_id], std::move(base), out);
        }
      }
    }
    offset += block_len;
  }

  for (unsigned i = 0; i != out.size(); ++i) {
    out[i].ngap_index = i;
  }
  return out;
}

bool endpoint_is_allowed(const std::string& value,
                         const std::string& local_ip,
                         const std::string& amf_ip,
                         const std::vector<std::string>& allowlisted_amf_ips)
{
  if (value.empty()) {
    return true;
  }
  return value == local_ip || value == amf_ip ||
         std::find(allowlisted_amf_ips.begin(), allowlisted_amf_ips.end(), value) != allowlisted_amf_ips.end();
}

} // namespace

std::vector<ngap_pcap_packet> ocudu::fbs::read_ngap_packets_from_pcap(const std::string& path)
{
  const std::vector<uint8_t> bytes = read_file(path);
  if (is_pcapng(bytes)) {
    return parse_pcapng(bytes);
  }
  return parse_pcap(bytes);
}

std::vector<ngap_pcap_packet>
ocudu::fbs::extract_ngap_payloads_from_packet(const uint8_t* data, size_t length, unsigned link_type, unsigned packet_index)
{
  ngap_pcap_packet base = {};
  base.packet_index     = packet_index;
  std::vector<ngap_pcap_packet> out;
  extract_frame_payload(data, length, link_type, std::move(base), out);
  for (unsigned i = 0; i != out.size(); ++i) {
    out[i].ngap_index = i;
  }
  return out;
}

bool ocudu::fbs::packet_endpoints_are_configured_lab_ips(const ngap_pcap_packet& packet,
                                                         const std::string&      local_ip,
                                                         const std::string&      amf_ip,
                                                         const std::vector<std::string>& allowlisted_amf_ips)
{
  return endpoint_is_allowed(packet.source_ip, local_ip, amf_ip, allowlisted_amf_ips) &&
         endpoint_is_allowed(packet.destination_ip, local_ip, amf_ip, allowlisted_amf_ips);
}
