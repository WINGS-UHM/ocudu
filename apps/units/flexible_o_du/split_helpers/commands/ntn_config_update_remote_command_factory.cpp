// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#include "ntn_config_update_remote_command_factory.h"

using namespace ocudu;

#ifndef OCUDU_HAS_ENTERPRISE_NTN

void ocudu::add_ntn_config_update_remote_command(application_unit_commands&            commands,
                                                 ocudu_ntn::ntn_configuration_manager& ntn_manager)
{
}

#endif // OCUDU_HAS_ENTERPRISE_NTN
