// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI

#include "sctp_server_test_helpers.h"
#include "ocudu/gateways/sctp_network_server_factory.h"
#include "ocudu/support/executors/inline_task_executor.h"
#include <arpa/inet.h>
#include <gtest/gtest.h>

using namespace ocudu;

class sctp_network_server_peer_test : public ::testing::Test
{
protected:
  sctp_network_server_peer_test()
  {
    ocudulog::fetch_basic_logger("SCTP-GW").set_level(ocudulog::basic_levels::debug);
    ocudulog::init();

    server_cfg1.sctp.if_name        = "SERVER1";
    server_cfg1.sctp.ppid           = XNAP_PPID;
    server_cfg1.sctp.bind_addresses = {"127.0.0.1"};
    server_cfg1.sctp.bind_port      = 0;

    server_cfg2.sctp.if_name        = "SERVER2";
    server_cfg2.sctp.ppid           = XNAP_PPID;
    server_cfg2.sctp.bind_addresses = {"127.0.0.2"};
    server_cfg2.sctp.bind_port      = 0;

    server_cfg3.sctp.if_name        = "SERVER3";
    server_cfg3.sctp.ppid           = XNAP_PPID;
    server_cfg3.sctp.bind_addresses = {"127.0.0.3"};
    server_cfg3.sctp.bind_port      = 0;
  }

  ~sctp_network_server_peer_test() override { ocudulog::flush(); }

  sctp_network_server_config server_cfg1{{}, broker1, io_rx_executor, assoc_factory1};
  sctp_network_server_config server_cfg2{{}, broker2, io_rx_executor, assoc_factory2};
  sctp_network_server_config server_cfg3{{}, broker3, io_rx_executor, assoc_factory3};

  dummy_io_broker broker1;
  dummy_io_broker broker2;
  dummy_io_broker broker3;

  inline_task_executor                 io_rx_executor;
  std::unique_ptr<sctp_network_server> server1;
  std::unique_ptr<sctp_network_server> server2;
  std::unique_ptr<sctp_network_server> server3;

  std::string server1_addr_str = "127.0.0.1";
  std::string server2_addr_str = "127.0.0.2";
  std::string server3_addr_str = "127.0.0.3";

  association_factory assoc_factory1;
  association_factory assoc_factory2;
  association_factory assoc_factory3;
};

TEST_F(sctp_network_server_peer_test, when_config_is_valid_then_server_is_created_successfully)
{
  server1 = create_sctp_network_server(server_cfg1);
  server2 = create_sctp_network_server(server_cfg2);
  server3 = create_sctp_network_server(server_cfg3);
  ASSERT_NE(server1, nullptr);
  ASSERT_NE(server2, nullptr);
  ASSERT_NE(server3, nullptr);
}

TEST_F(sctp_network_server_peer_test, when_association_requested_association_initiates_successfully)
{
  std::array<uint8_t, 4> data = {0x00, 0x01, 0x02, 0x03};
  span<uint8_t>          data_span(data.data(), data.size());

  server1 = create_sctp_network_server(server_cfg1);
  server2 = create_sctp_network_server(server_cfg2);
  server3 = create_sctp_network_server(server_cfg3);
  ASSERT_NE(server1, nullptr);
  ASSERT_NE(server2, nullptr);
  ASSERT_NE(server3, nullptr);

  server1->listen();
  server2->listen();
  server3->listen();

  transport_layer_address addr1 = transport_layer_address::create_from_string(server1_addr_str);
  std::optional<uint16_t> port1 = server1->get_listen_port();
  ASSERT_TRUE(port1);
  addr1.set_port(*port1);

  transport_layer_address addr2 = transport_layer_address::create_from_string(server2_addr_str);
  std::optional<uint16_t> port2 = server2->get_listen_port();
  ASSERT_TRUE(port2);
  addr2.set_port(*port2);

  transport_layer_address addr3 = transport_layer_address::create_from_string(server3_addr_str);
  std::optional<uint16_t> port3 = server3->get_listen_port();
  ASSERT_TRUE(port3);
  addr3.set_port(*port3);

  // Create associations between S1 <-> S2
  {
    byte_buffer msg;
    ASSERT_TRUE(msg.append(data_span));
    server1->init_association_with_msg(addr2, std::move(msg));
  }

  ASSERT_EQ(0, assoc_factory1.association_count());
  ASSERT_EQ(0, assoc_factory2.association_count());
  ASSERT_EQ(0, assoc_factory3.association_count());
  broker1.handle_receive(); // CONN_UP
  broker2.handle_receive(); // CONN_UP
  broker2.handle_receive(); // RX DATA
  ASSERT_EQ(1, assoc_factory1.association_count());
  ASSERT_EQ(1, assoc_factory2.association_count());
  ASSERT_EQ(0, assoc_factory3.association_count());

  // Create associations between S1 <-> S3
  {
    byte_buffer msg;
    ASSERT_TRUE(msg.append(data_span));
    server1->init_association_with_msg(addr3, std::move(msg));
  }

  broker1.handle_receive(); // CONN_UP
  broker3.handle_receive(); // CONN_UP
  broker3.handle_receive(); // RX DATA
  ASSERT_EQ(2, assoc_factory1.association_count());
  ASSERT_EQ(1, assoc_factory2.association_count());
  ASSERT_EQ(1, assoc_factory3.association_count());

  // Create associations between S2 <-> S3
  {
    byte_buffer msg;
    ASSERT_TRUE(msg.append(data_span));
    server2->init_association_with_msg(addr3, std::move(msg));
  }
  broker2.handle_receive(); // CONN_UP
  broker3.handle_receive(); // CONN_UP
  broker3.handle_receive(); // RX DATA
  ASSERT_EQ(2, assoc_factory1.association_count());
  ASSERT_EQ(2, assoc_factory2.association_count());
  ASSERT_EQ(2, assoc_factory3.association_count());

  /// Close servers
  server1.reset();
  server2.reset();
  server3.reset();
}
