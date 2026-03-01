// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#include "ocudu/hal/phy/upper/channel_processors/hw_accelerator_factories.h"

using namespace ocudu;
using namespace hal;

#ifndef OCUDU_HAS_ENTERPRISE

std::shared_ptr<hw_accelerator_pdsch_enc_factory>
ocudu::hal::create_bbdev_pdsch_enc_acc_factory(const bbdev_hwacc_pdsch_enc_factory_configuration& accelerator_config)
{
  return nullptr;
}

#endif // OCUDU_HAS_ENTERPRISE
