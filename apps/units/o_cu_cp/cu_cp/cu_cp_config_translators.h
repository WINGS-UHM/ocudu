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

#include "ocudu/cu_cp/cu_cp_configuration.h"
#include "ocudu/ngap/gateways/n2_connection_client_factory.h"

namespace ocudu {

struct cu_cp_unit_config;
struct cu_cp_unit_amf_config_item;
struct worker_manager_config;

/// Converts and returns the given gnb application configuration to a CU-CP configuration.
ocucp::cu_cp_configuration generate_cu_cp_config(const cu_cp_unit_config& cu_cfg);

/// Converts CU-CP configuration into N2 connection client.
ocucp::n2_connection_client_config generate_n2_client_config(bool                              no_core,
                                                             const cu_cp_unit_amf_config_item& amf_cfg,
                                                             dlt_pcap&                         pcap_writer,
                                                             io_broker&                        broker,
                                                             task_executor&                    io_rx_executor);

/// Fills the CU-CP worker manager parameters of the given worker manager configuration.
void fill_cu_cp_worker_manager_config(worker_manager_config& config, const cu_cp_unit_config& unit_cfg);

} // namespace ocudu
