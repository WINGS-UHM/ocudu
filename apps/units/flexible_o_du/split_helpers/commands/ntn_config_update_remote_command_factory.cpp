/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "ntn_config_update_remote_command_factory.h"

using namespace ocudu;

#ifndef OCUDU_HAS_ENTERPRISE_NTN

void ocudu::add_ntn_config_update_remote_command(application_unit_commands&            commands,
                                                 ocudu_ntn::ntn_configuration_manager& ntn_manager)
{
}

#endif // OCUDU_HAS_ENTERPRISE_NTN
