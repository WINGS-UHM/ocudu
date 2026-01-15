/*
 *
 * Copyright 2021-2025 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "ocudu/ocudulog/ocudulog.h"
#include "ocudu/support/io/sctp_socket.h"
#include <arpa/inet.h>
#include <cstring>
#include <gtest/gtest.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <optional>
#include <sys/socket.h>

using namespace ocudu;

class sctp_socket_test : public ::testing::Test
{
protected:
  sctp_socket_test() { ocudulog::init(); }

  ~sctp_socket_test() override { ocudulog::flush(); }

  static sctp_socket_params create_default_params()
  {
    sctp_socket_params params;
    params.if_name     = "test_sctp";
    params.ai_family   = AF_INET;
    params.ai_socktype = SOCK_SEQPACKET;
    return params;
  }

  /// Create an IPv4 loopback address with the given port
  static sockaddr_storage create_ipv4_loopback_addr(uint16_t port)
  {
    sockaddr_storage addr_storage = {};
    auto*            addr_in      = reinterpret_cast<sockaddr_in*>(&addr_storage);
    addr_in->sin_family           = AF_INET;
    addr_in->sin_port             = htons(port);
    addr_in->sin_addr.s_addr      = htonl(INADDR_LOOPBACK);
    return addr_storage;
  }

  /// Create an IPv6 loopback address with the given port
  static sockaddr_storage create_ipv6_loopback_addr(uint16_t port)
  {
    sockaddr_storage addr_storage = {};
    auto*            addr_in6     = reinterpret_cast<sockaddr_in6*>(&addr_storage);
    addr_in6->sin6_family         = AF_INET6;
    addr_in6->sin6_port           = htons(port);
    addr_in6->sin6_addr           = in6addr_loopback;
    return addr_storage;
  }

  /// Verify bound addresses using sctp_getladdrs.
  static void verify_bound_address_ipv4(int fd, std::optional<uint16_t> expected_port, in_addr_t expected_addr)
  {
    sockaddr* addrs = nullptr;
    int       count = sctp_getladdrs(fd, 0, &addrs);
    ASSERT_GT(count, 0);

    // Verify the first address matches expectations.
    auto* sin = reinterpret_cast<sockaddr_in*>(addrs);
    EXPECT_EQ(sin->sin_family, AF_INET);
    if (expected_port.has_value()) {
      EXPECT_EQ(ntohs(sin->sin_port), expected_port.value());
    } else {
      // Check that dynamic port was assigned.
      EXPECT_GT(ntohs(sin->sin_port), 0);
    }
    EXPECT_EQ(ntohl(sin->sin_addr.s_addr), expected_addr);

    sctp_freeladdrs(addrs);
  }

  /// Verify bound addresses for IPv6 using sctp_getladdrs.
  static void verify_bound_address_ipv6(int fd, std::optional<uint16_t> expected_port, const in6_addr& expected_addr)
  {
    sockaddr* addrs = nullptr;
    int       count = sctp_getladdrs(fd, 0, &addrs);
    ASSERT_GT(count, 0);

    // Verify the first address matches expectations.
    auto* sin6 = reinterpret_cast<sockaddr_in6*>(addrs);
    EXPECT_EQ(sin6->sin6_family, AF_INET6);
    if (expected_port.has_value()) {
      EXPECT_EQ(ntohs(sin6->sin6_port), expected_port.value());
    } else {
      // Check that dynamic port was assigned.
      EXPECT_GT(ntohs(sin6->sin6_port), 0);
    }
    EXPECT_EQ(std::memcmp(&sin6->sin6_addr, &expected_addr, sizeof(expected_addr)), 0);

    sctp_freeladdrs(addrs);
  }
};

/// Test that creating a socket with empty interface name fails.
TEST_F(sctp_socket_test, create_fails_with_empty_if_name)
{
  sctp_socket_params params = create_default_params();
  params.if_name            = "";

  auto result = sctp_socket::create(params);
  ASSERT_FALSE(result.has_value());
}

/// Test that a socket can be successfully created with valid parameters.
TEST_F(sctp_socket_test, create_succeeds_with_valid_params)
{
  sctp_socket_params params = create_default_params();

  auto result = sctp_socket::create(params);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result.value().is_open());
  EXPECT_TRUE(result.value().fd().is_open());
}

/// Test that a socket can be created with IPv6 family.
TEST_F(sctp_socket_test, create_succeeds_with_ipv6)
{
  sctp_socket_params params = create_default_params();
  params.ai_family          = AF_INET6;

  auto result = sctp_socket::create(params);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result.value().is_open());
}

/// Test that close() works properly.
TEST_F(sctp_socket_test, close_socket)
{
  sctp_socket_params params = create_default_params();

  auto result = sctp_socket::create(params);
  ASSERT_TRUE(result.has_value());

  sctp_socket& sock = result.value();
  EXPECT_TRUE(sock.is_open());

  EXPECT_TRUE(sock.close());
  EXPECT_FALSE(sock.is_open());
}

/// Test that close() on an already closed socket still returns true.
TEST_F(sctp_socket_test, close_already_closed_socket)
{
  sctp_socket_params params = create_default_params();

  auto result = sctp_socket::create(params);
  ASSERT_TRUE(result.has_value());

  sctp_socket& sock = result.value();
  EXPECT_TRUE(sock.is_open());

  EXPECT_TRUE(sock.close());
  EXPECT_FALSE(sock.is_open());

  EXPECT_TRUE(sock.close());
}

/// Test socket creation with all options set.
TEST_F(sctp_socket_test, create_with_all_options)
{
  sctp_socket_params params = create_default_params();
  params.reuse_addr         = true;
  params.non_blocking_mode  = true;
  params.rx_timeout         = std::chrono::seconds(1);
  params.rto_initial        = std::chrono::milliseconds(2);
  params.rto_min            = std::chrono::milliseconds(3);
  params.rto_max            = std::chrono::milliseconds(4);
  params.init_max_attempts  = 5;
  params.max_init_timeo     = std::chrono::milliseconds(6);
  params.hb_interval        = std::chrono::milliseconds(7);
  params.assoc_max_rxt      = 8;
  params.nodelay            = true;

  auto result = sctp_socket::create(params);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result.value().is_open());
}

/// Test bindx to loopback address with a dynamic port (when port is not set then OS picks the port number).
TEST_F(sctp_socket_test, bindx_to_loopback_with_dynamic_port)
{
  sctp_socket_params params = create_default_params();

  auto result = sctp_socket::create(params);
  ASSERT_TRUE(result.has_value());

  sctp_socket& sock = result.value();

  sockaddr_storage addr_storage = create_ipv4_loopback_addr(0);

  EXPECT_TRUE(sock.bindx({addr_storage}, ""));

  ASSERT_TRUE(sock.get_listen_port().has_value());
  EXPECT_GT(sock.get_listen_port().value(), 0);

  verify_bound_address_ipv4(sock.fd().value(), std::nullopt, INADDR_LOOPBACK);
}

/// Test bindx to loopback address with a specific port.
TEST_F(sctp_socket_test, bindx_to_loopback_with_specific_port)
{
  sctp_socket_params params = create_default_params();
  params.reuse_addr         = true;

  auto result = sctp_socket::create(params);
  ASSERT_TRUE(result.has_value());

  sctp_socket& sock = result.value();

  sockaddr_storage addr_storage = create_ipv4_loopback_addr(12345);

  EXPECT_TRUE(sock.bindx({addr_storage}, ""));

  ASSERT_TRUE(sock.get_listen_port().has_value());
  EXPECT_EQ(sock.get_listen_port().value(), 12345);

  verify_bound_address_ipv4(sock.fd().value(), 12345, INADDR_LOOPBACK);
}

/// Test bindx to loopback address with a dynamic port (when port is not set then OS picks the port number).
TEST_F(sctp_socket_test, bindx_to_loopback_with_dynamic_port_ipv6)
{
  sctp_socket_params params = create_default_params();
  params.ai_family          = AF_INET6;

  auto result = sctp_socket::create(params);
  ASSERT_TRUE(result.has_value());

  sctp_socket& sock = result.value();

  sockaddr_storage addr_storage = create_ipv6_loopback_addr(0);

  EXPECT_TRUE(sock.bindx({addr_storage}, ""));

  ASSERT_TRUE(sock.get_listen_port().has_value());
  EXPECT_GT(sock.get_listen_port().value(), 0);

  verify_bound_address_ipv6(sock.fd().value(), std::nullopt, in6addr_loopback);
}

/// Test bindx to loopback address with a specific port.
TEST_F(sctp_socket_test, bindx_to_loopback_with_specific_port_ipv6)
{
  sctp_socket_params params = create_default_params();
  params.ai_family          = AF_INET6;
  params.reuse_addr         = true;

  auto result = sctp_socket::create(params);
  ASSERT_TRUE(result.has_value());

  sctp_socket& sock = result.value();

  sockaddr_storage addr_storage = create_ipv6_loopback_addr(12345);

  EXPECT_TRUE(sock.bindx({addr_storage}, ""));

  ASSERT_TRUE(sock.get_listen_port().has_value());
  EXPECT_EQ(sock.get_listen_port().value(), 12345);

  verify_bound_address_ipv6(sock.fd().value(), 12345, in6addr_loopback);
}

/// Test listen on a bound socket.
TEST_F(sctp_socket_test, listen_on_bound_socket)
{
  sctp_socket_params params = create_default_params();

  auto result = sctp_socket::create(params);
  ASSERT_TRUE(result.has_value());

  sctp_socket& sock = result.value();

  sockaddr_storage addr_storage = create_ipv4_loopback_addr(0);

  ASSERT_TRUE(sock.bindx({addr_storage}, ""));
  EXPECT_TRUE(sock.listen());
}

/// Test listen on an unbound socket. TO-DO: Should this be allowed?!
TEST_F(sctp_socket_test, listen_on_unbound_socket)
{
  sctp_socket_params params = create_default_params();

  auto result = sctp_socket::create(params);
  ASSERT_TRUE(result.has_value());

  sctp_socket& sock = result.value();

  EXPECT_TRUE(sock.listen());
}

/// Test get_listen_port returns nullopt for an unbound socket.
TEST_F(sctp_socket_test, get_listen_port_returns_nullopt_for_unbound_socket)
{
  sctp_socket_params params = create_default_params();

  auto result = sctp_socket::create(params);
  ASSERT_TRUE(result.has_value());

  sctp_socket& sock = result.value();

  ASSERT_TRUE(sock.get_listen_port().has_value());
  EXPECT_EQ(sock.get_listen_port().value(), 0);
}

/// Test listen fails on a closed socket.
TEST_F(sctp_socket_test, listen_fails_on_closed_socket)
{
  sctp_socket_params params = create_default_params();

  auto result = sctp_socket::create(params);
  ASSERT_TRUE(result.has_value());

  sctp_socket& sock = result.value();
  sock.close();

  EXPECT_FALSE(sock.listen());
}

/// Test get_listen_port returns nullopt for a closed socket.
TEST_F(sctp_socket_test, get_listen_port_returns_nullopt_for_closed_socket)
{
  sctp_socket_params params = create_default_params();

  auto result = sctp_socket::create(params);
  ASSERT_TRUE(result.has_value());

  sctp_socket& sock = result.value();

  sockaddr_storage addr_storage = create_ipv4_loopback_addr(0);

  ASSERT_TRUE(sock.bindx({addr_storage}, ""));

  EXPECT_TRUE(sock.get_listen_port().has_value());
  EXPECT_GT(sock.get_listen_port().value(), 0);

  sock.close();

  EXPECT_FALSE(sock.get_listen_port().has_value());
}

/// Test bindx with an empty address list (should succeed/skip). TO-DO: Should this be allowed?!
TEST_F(sctp_socket_test, bindx_with_empty_list)
{
  sctp_socket_params params = create_default_params();
  params.reuse_addr         = true;

  auto result = sctp_socket::create(params);
  ASSERT_TRUE(result.has_value());

  sctp_socket& sock = result.value();

  std::vector<sockaddr_storage> addrs;
  EXPECT_TRUE(sock.bindx(addrs, ""));
}

/// Test bindx with a single address.
TEST_F(sctp_socket_test, bindx_with_single_address)
{
  sctp_socket_params params = create_default_params();
  params.reuse_addr         = true;

  auto result = sctp_socket::create(params);
  ASSERT_TRUE(result.has_value());

  sctp_socket& sock = result.value();

  std::vector<sockaddr_storage> addrs;
  addrs.push_back(create_ipv4_loopback_addr(0));

  EXPECT_TRUE(sock.bindx(addrs, ""));

  // Verify bound address using sctp_getladdrs
  verify_bound_address_ipv4(sock.fd().value(), std::nullopt, INADDR_LOOPBACK);
}

/// Test bindx fails on a closed socket.
TEST_F(sctp_socket_test, bindx_fails_on_closed_socket)
{
  sctp_socket_params params = create_default_params();

  auto result = sctp_socket::create(params);
  ASSERT_TRUE(result.has_value());

  sctp_socket& sock = result.value();
  sock.close();

  std::vector<sockaddr_storage> addrs;
  addrs.push_back(create_ipv4_loopback_addr(0));

  EXPECT_FALSE(sock.bindx(addrs, ""));
}

/// Test connectx fails on a closed socket.
TEST_F(sctp_socket_test, connectx_fails_on_closed_socket)
{
  sctp_socket_params params = create_default_params();

  auto result = sctp_socket::create(params);
  ASSERT_TRUE(result.has_value());

  sctp_socket& sock = result.value();
  sock.close();

  sockaddr_storage addr_storage = create_ipv4_loopback_addr(12345);

  EXPECT_FALSE(sock.connectx({addr_storage}));
}

/// Test connectx fails with an empty address list.
TEST_F(sctp_socket_test, connectx_fails_with_empty_list)
{
  sctp_socket_params params = create_default_params();

  auto result = sctp_socket::create(params);
  ASSERT_TRUE(result.has_value());

  sctp_socket& sock = result.value();

  std::vector<sockaddr_storage> addrs;
  EXPECT_FALSE(sock.connectx(addrs));
}

/// TO-DO:
/// Tests for sctp_connectx() and bindx() with multiple addresses are needed, but this is not possible as long as we are
/// limited to localhost in unittests.
