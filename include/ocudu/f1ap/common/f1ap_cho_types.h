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

namespace ocudu {

/// \brief CHO trigger values for the inter-DU path (TS 38.473 Section 9.3.1.x).
/// Only cho_initiation and cho_replace are valid over the F1AP inter-DU interface.
enum class f1ap_cho_trigger { cho_initiation = 0, cho_replace };

} // namespace ocudu
