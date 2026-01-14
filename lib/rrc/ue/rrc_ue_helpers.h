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

#include "rrc_ue_logger.h"

namespace ocudu::ocucp {

// Logging.
typedef enum { Rx = 0, Tx } direction_t;

template <class T>
void log_rrc_message(rrc_ue_logger&    logger,
                     const direction_t dir,
                     byte_buffer_view  pdu,
                     const T&          msg,
                     srb_id_t          srb_id,
                     const char*       msg_type);

} // namespace ocudu::ocucp
