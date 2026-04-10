// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI

#include "ocudu/support/io/transport_layer_address.h"
#include "ocudu/support/ocudu_assert.h"

using namespace ocudu;

transport_layer_address::transport_layer_address(const struct sockaddr& addr_, socklen_t socklen) : addrlen(socklen)
{
  std::memcpy((struct sockaddr*)&addr_storage, &addr_, addrlen);
}

transport_layer_address transport_layer_address::create_from_sockaddr(const struct sockaddr& sockaddr,
                                                                      socklen_t              addr_len)
{
  return transport_layer_address(sockaddr, addr_len);
}

transport_layer_address transport_layer_address::create_from_sockaddr(native_type addr)
{
  return transport_layer_address(*addr.addr, addr.addrlen);
}

transport_layer_address transport_layer_address::create_from_string(const std::string& ip_str)
{
  ::addrinfo* results;

  int err = ::getaddrinfo(ip_str.c_str(), nullptr, nullptr, &results);
  ocudu_assert(err == 0, "Getaddrinfo error: {} - {}", ip_str, ::gai_strerror(err));

  transport_layer_address res(*results->ai_addr, results->ai_addrlen);

  ::freeaddrinfo(results);

  return res;
}

transport_layer_address transport_layer_address::create_from_bitstring(const std::string& bit_str)
{
  ocudu_assert(bit_str.length() < 160, "Combined IPv4 and IPv6 addresses are currently not supported");

  // See TS 38.414: 32 bits in case of IPv4 address according to IETF RFC 791.
  if (bit_str.length() == 32) {
    std::string ip_str = fmt::format("{}.{}.{}.{}",
                                     std::stoi(bit_str.substr(0, 8), nullptr, 2),
                                     std::stoi(bit_str.substr(8, 8), nullptr, 2),
                                     std::stoi(bit_str.substr(16, 8), nullptr, 2),
                                     std::stoi(bit_str.substr(24, 8), nullptr, 2));
    return create_from_string(ip_str);
  }

  // See TS 38.414: 128 bits in case of IPv6 address according to IETF RFC 8200.
  if (bit_str.length() == 128) {
    std::string ip_str = fmt::format("{:x}:{:x}:{:x}:{:x}:{:x}:{:x}:{:x}:{:x}",
                                     std::stoi(bit_str.substr(0, 16), nullptr, 2),
                                     std::stoi(bit_str.substr(16, 16), nullptr, 2),
                                     std::stoi(bit_str.substr(32, 16), nullptr, 2),
                                     std::stoi(bit_str.substr(48, 16), nullptr, 2),
                                     std::stoi(bit_str.substr(64, 16), nullptr, 2),
                                     std::stoi(bit_str.substr(80, 16), nullptr, 2),
                                     std::stoi(bit_str.substr(96, 16), nullptr, 2),
                                     std::stoi(bit_str.substr(112, 16), nullptr, 2));
    return create_from_string(ip_str);
  }

  // See TS 38.414: 160 bits if both IPv4 and IPv6 addresses are signalled, in which case the IPv4 address is contained
  // in the first 32 bits.
  // TODO: Support 160 bit transport layer addresses

  return create_from_string("");
}

std::string transport_layer_address::to_bitstring() const
{
  const auto* saddr = reinterpret_cast<const sockaddr*>(&addr_storage);

  std::string bitstr;

  if (saddr->sa_family == AF_INET) {
    const auto* addr  = reinterpret_cast<const sockaddr_in*>(saddr);
    const auto* bytes = reinterpret_cast<const uint8_t*>(&addr->sin_addr);

    for (size_t i = 0; i < 4; ++i) {
      bitstr += fmt::format("{:08b}", bytes[i]);
    }
    return bitstr;
  }

  if (saddr->sa_family == AF_INET6) {
    const auto* addr  = reinterpret_cast<const sockaddr_in6*>(saddr);
    const auto* bytes = reinterpret_cast<const uint8_t*>(&addr->sin6_addr);

    for (size_t i = 0; i < 16; ++i) {
      bitstr += fmt::format("{:08b}", bytes[i]);
    }
    return bitstr;
  }

  ocudu_assertion_failure("Wrong address family");
  return {};
}

bool transport_layer_address::operator==(const transport_layer_address& other) const
{
  if (empty() && other.empty()) {
    return true;
  }
  return sockaddr_storage_equal(addr_storage, other.addr_storage);
}

bool transport_layer_address::operator<(const transport_layer_address& other) const
{
  return sockaddr_storage_less{}(addr_storage, other.addr_storage);
}
