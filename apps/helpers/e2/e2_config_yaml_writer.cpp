// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#include "e2_config_yaml_writer.h"
#include "ocudu/e2/e2ap_configuration.h"

using namespace ocudu;

void ocudu::fill_e2_config_in_yaml_schema(YAML::Node node, const e2_config& config)
{
  node["addrs"]                  = config.ip_addrs;
  node["port"]                   = config.port;
  node["bind_addrs"]             = config.bind_addrs;
  node["sctp_rto_initial"]       = config.sctp_rto_initial;
  node["sctp_rto_min"]           = config.sctp_rto_min;
  node["sctp_rto_max"]           = config.sctp_rto_max;
  node["sctp_init_max_attempts"] = config.sctp_init_max_attempts;
  node["sctp_max_init_timeo"]    = config.sctp_max_init_timeo;
  node["e2sm_kpm_enabled"]       = config.e2sm_kpm_enabled;
  node["e2sm_rc_enabled"]        = config.e2sm_rc_enabled;
  node["e2sm_ccc_enabled"]       = config.e2sm_ccc_enabled;
}
