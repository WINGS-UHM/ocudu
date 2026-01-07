/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#pragma once

#include "ocudu/cu_up/cu_up_executor_mapper.h"
#include "ocudu/e1ap/cu_up/e1ap_configuration.h"
#include "ocudu/e1ap/cu_up/e1ap_cu_up.h"
#include "ocudu/e1ap/gateways/e1_connection_client.h"
#include "ocudu/f1u/cu_up/f1u_gateway.h"
#include "ocudu/gtpu/gtpu_config.h"
#include "ocudu/gtpu/gtpu_gateway.h"
#include "ocudu/pcap/dlt_pcap.h"
#include "ocudu/ran/gnb_cu_up_id.h"
#include "ocudu/support/timers.h"
#include <map>

namespace ocudu {

class io_broker;

namespace ocuup {

struct network_interface_config {
  /// Local IP address to bind for connection from DU to receive uplink user-plane traffic.
  std::string f1u_bind_addr = "127.0.2.1";

  /// Local port to bind for connection from DU to receive uplink user-plane traffic.
  int f1u_bind_port = GTPU_PORT;
};

struct n3_interface_config {
  int                       upf_port = GTPU_PORT;      // TS 29.281 Sec. 4.4.2.3 Encapsulated T-PDUs
  std::chrono::milliseconds gtpu_reordering_timer;     // N3 reordering timer
  std::chrono::milliseconds gtpu_rate_limiting_period; // N3 token bucket rate limiting period.
  bool                      gtpu_ignore_ue_ambr;       // Ignore DL UE-AMBR.
  uint32_t                  gtpu_queue_size;           // GTP-U queue size in PDUs.
  uint32_t                  gtpu_batch_size;           // Maximum number of GTP-U PDUs processed in a batch.
  bool                      warn_on_drop;
};

struct cu_up_test_mode_config {
  bool                      enabled           = false;
  bool                      integrity_enabled = true;
  bool                      ciphering_enabled = true;
  uint16_t                  nea_algo          = 2;
  uint16_t                  nia_algo          = 2;
  uint64_t                  ue_ambr           = 40000000000;
  std::chrono::milliseconds attach_detach_period{0};
  std::chrono::milliseconds reestablish_period{0};
};

/// CU-UP configuration.
struct cu_up_config {
  /// 5QI as key.
  std::map<five_qi_t, cu_up_qos_config> qos;
  /// N3 configuration.
  n3_interface_config n3_cfg;
  /// Test mode configuration.
  cu_up_test_mode_config test_mode_cfg;
  /// gNodeB identifier.
  gnb_id_t gnb_id = {411, 22};
  /// CU-UP identifier.
  gnb_cu_up_id_t cu_up_id = gnb_cu_up_id_t::min;
  /// CU-UP name.
  std::string cu_up_name = "ocuup_01";
  /// E1AP configuration.
  e1ap_configuration e1ap;
  /// Full PLMN as string (without possible filler digit) e.g. "00101".
  std::string plmn = "00101";
  /// CU-UP statistics report period in seconds.
  std::chrono::seconds statistics_report_period;
};

/// CU-UP dependencies.
struct cu_up_dependencies {
  /// Executor mapper.
  cu_up_executor_mapper* exec_mapper = nullptr;
  /// F1-U gateway.
  f1u_cu_up_gateway* f1u_gateway = nullptr;
  /// Time manager.
  timer_manager* timers = nullptr;
  /// PCAP.
  dlt_pcap* gtpu_pcap = nullptr;
  /// PDCP metrics notifier.
  pdcp_metrics_notifier* pdcp_metric_notifier = nullptr;
  /// E1AP connection client.
  e1_connection_client* e1_conn_client = nullptr;
  /// NG-U gateways
  std::vector<gtpu_gateway*> ngu_gws;
};

} // namespace ocuup
} // namespace ocudu

namespace fmt {

// Network config formatter
template <>
struct formatter<ocudu::ocuup::network_interface_config> {
  template <typename ParseContext>
  auto parse(ParseContext& ctx)
  {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(const ocudu::ocuup::network_interface_config& cfg, FormatContext& ctx) const
  {
    return format_to(ctx.out(), "f1u_bind_addr={} f1u_bind_port={}", cfg.f1u_bind_addr, cfg.f1u_bind_port);
  }
};
} // namespace fmt
