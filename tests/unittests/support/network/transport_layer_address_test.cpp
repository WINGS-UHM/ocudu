// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI

#include "tests/test_doubles/utils/test_rng.h"
#include "ocudu/support/io/transport_layer_address.h"
#include <arpa/inet.h>
#include <gtest/gtest.h>

using namespace ocudu;

std::string create_random_ipv4_string()
{
  std::vector<uint8_t> nums = test_rng::vector_of_uniform_ints<uint8_t>(4);
  return fmt::format("{}.{}.{}.{}", nums[0], nums[1], nums[2], nums[3]);
}

std::string create_random_ipv6_string()
{
  std::vector<uint16_t> nums = test_rng::vector_of_uniform_ints<uint16_t>(8);
  return fmt::format("{:x}:{:x}:{:x}:{:x}:{:x}:{:x}:{:x}:{:x}",
                     nums[0],
                     nums[1],
                     nums[2],
                     nums[3],
                     nums[4],
                     nums[5],
                     nums[6],
                     nums[7]);
}

TEST(transport_layer_address_test, empty_address)
{
  transport_layer_address addr{}, addr2{};
  ASSERT_EQ(fmt::format("{}", addr), "invalid");
  ASSERT_EQ(addr, addr2);
}

TEST(transport_layer_address_test, conversion_to_ipv4_string)
{
  std::string ipv4_str = create_random_ipv4_string();
  auto        addr     = transport_layer_address::create_from_string(ipv4_str);
  ASSERT_EQ(addr.to_string(), ipv4_str);
  ASSERT_EQ(fmt::format("{}", addr), ipv4_str);
}

TEST(transport_layer_address_test, conversion_to_ipv6_string)
{
  std::string ipv6_str = create_random_ipv6_string();
  auto        addr     = transport_layer_address::create_from_string(ipv6_str);
  ASSERT_EQ(addr.to_string(), ipv6_str);
  ASSERT_EQ(fmt::format("{}", addr), ipv6_str);
}

TEST(transport_layer_address_test, ipv4_address_comparison)
{
  std::string ipv4_str1 = create_random_ipv4_string();
  std::string ipv4_str2 = create_random_ipv4_string();
  auto        addr1     = transport_layer_address::create_from_string(ipv4_str1);
  auto        addr2     = transport_layer_address::create_from_string(ipv4_str2);
  ASSERT_EQ(addr1, ipv4_str1);
  ASSERT_EQ(addr2, ipv4_str2);
}

TEST(transport_layer_address_test, ipv6_address_comparison)
{
  std::string ipv6_str1 = create_random_ipv6_string();
  std::string ipv6_str2 = create_random_ipv6_string();
  auto        addr1     = transport_layer_address::create_from_string(ipv6_str1);
  auto        addr2     = transport_layer_address::create_from_string(ipv6_str2);
  ASSERT_EQ(addr1, ipv6_str1);
  ASSERT_EQ(addr2, ipv6_str2);
}

TEST(transport_layer_address_test, ipv4_equal_ignores_sockaddr_padding)
{
  // Build a sockaddr_in with garbage in the sin_zero padding field.
  sockaddr_in sa_dirty = {};
  sa_dirty.sin_family  = AF_INET;
  sa_dirty.sin_port    = htons(5000);
  inet_pton(AF_INET, "10.0.0.1", &sa_dirty.sin_addr);
  std::memset(sa_dirty.sin_zero, 0xff, sizeof(sa_dirty.sin_zero));

  // Build a clean sockaddr_in with the same address but zero padding.
  sockaddr_in sa_clean = {};
  sa_clean.sin_family  = AF_INET;
  sa_clean.sin_port    = htons(5000);
  inet_pton(AF_INET, "10.0.0.1", &sa_clean.sin_addr);

  auto addr_dirty =
      transport_layer_address::create_from_sockaddr(*reinterpret_cast<sockaddr*>(&sa_dirty), sizeof(sa_dirty));
  auto addr_clean =
      transport_layer_address::create_from_sockaddr(*reinterpret_cast<sockaddr*>(&sa_clean), sizeof(sa_clean));

  // Field-by-field comparison must treat these as equal despite different padding bytes.
  ASSERT_EQ(addr_dirty, addr_clean);
  ASSERT_FALSE(addr_dirty < addr_clean);
  ASSERT_FALSE(addr_clean < addr_dirty);
}

TEST(transport_layer_address_test, ipv4_is_always_different_from_ipv6)
{
  std::string ipv4_str = create_random_ipv4_string();
  std::string ipv6_str = create_random_ipv6_string();
  auto        addr1    = transport_layer_address::create_from_string(ipv4_str);
  auto        addr2    = transport_layer_address::create_from_string(ipv6_str);
  ASSERT_NE(addr1, addr2);
}

TEST(transport_layer_address_test, ipv6_bitstring_with_compressed_address)
{
  // This IPv6 address contains '::' compression
  std::string ipv6_str = "2001:db8::1";

  auto addr = transport_layer_address::create_from_string(ipv6_str);

  // This should NOT throw and should produce a valid 128-bit string
  std::string bitstr;
  ASSERT_NO_THROW(bitstr = addr.to_bitstring());

  ASSERT_EQ(bitstr.size(), 128);
  ASSERT_EQ(bitstr,
            "0010000000000001" // 2001
            "0000110110111000" // 0db8
            "0000000000000000" // 0000
            "0000000000000000" // 0000
            "0000000000000000" // 0000
            "0000000000000000" // 0000
            "0000000000000000" // 0000
            "0000000000000001" // 0001
  );
}

TEST(transport_layer_address_test, ipv6_bitstring_with_compressable_address)
{
  // This IPv6 address can/will be compressed with '::'
  std::string ipv6_str = "2001:0db8:0000:0000:0000:0000:0000:0001";

  auto addr = transport_layer_address::create_from_string(ipv6_str);

  // This should NOT throw and should produce a valid 128-bit string
  std::string bitstr;
  ASSERT_NO_THROW(bitstr = addr.to_bitstring());

  ASSERT_EQ(bitstr.size(), 128);
  ASSERT_EQ(bitstr,
            "0010000000000001" // 2001
            "0000110110111000" // 0db8
            "0000000000000000" // 0000
            "0000000000000000" // 0000
            "0000000000000000" // 0000
            "0000000000000000" // 0000
            "0000000000000000" // 0000
            "0000000000000001" // 0001
  );
}

TEST(transport_layer_address_test, ipv6_bitstring_with_localhost)
{
  std::string ipv6_str = "::1";

  auto addr = transport_layer_address::create_from_string(ipv6_str);

  // This should NOT throw and should produce a valid 128-bit string
  std::string bitstr;
  ASSERT_NO_THROW(bitstr = addr.to_bitstring());

  ASSERT_EQ(bitstr.size(), 128);
  ASSERT_EQ(bitstr,
            "0000000000000000000000000000000000000000000000000000000000000000"
            "0000000000000000000000000000000000000000000000000000000000000001");
}

TEST(transport_layer_address_test, ipv6_bitstring_with_all_networks)
{
  std::string ipv6_str = "::";

  auto addr = transport_layer_address::create_from_string(ipv6_str);

  // This should NOT throw and should produce a valid 128-bit string
  std::string bitstr;
  ASSERT_NO_THROW(bitstr = addr.to_bitstring());

  ASSERT_EQ(bitstr.size(), 128);
  ASSERT_EQ(bitstr,
            "0000000000000000000000000000000000000000000000000000000000000000"
            "0000000000000000000000000000000000000000000000000000000000000000");
}
