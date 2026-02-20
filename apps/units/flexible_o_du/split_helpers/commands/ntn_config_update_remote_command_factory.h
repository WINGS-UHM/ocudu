// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#pragma once

namespace ocudu {

struct application_unit_commands;

namespace ocudu_ntn {
class ntn_configuration_manager;
} // namespace ocudu_ntn

/// \brief Adds NTN config update remote command.
/// \param commands Application unit commands.
/// \param ntn_manager NTN config manager.
void add_ntn_config_update_remote_command(application_unit_commands&            commands,
                                          ocudu_ntn::ntn_configuration_manager& ntn_manager);

} // namespace ocudu
