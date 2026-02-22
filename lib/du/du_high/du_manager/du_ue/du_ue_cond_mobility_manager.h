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
namespace odu {

/// \brief Tracks conditional mobility state for a UE in the DU.
class du_ue_cond_mobility_manager
{
public:
  /// \brief Mark that an Access Success is expected for this UE upon C-RNTI CE reception.
  void set_success_access_required();

  /// \brief Handle a C-RNTI CE indication.
  ///
  /// Returns true and clears the flag if an Access Success was pending for this UE.
  /// Returns false without modifying state if no Access Success was expected.
  bool handle_crnti_ce_indication();

private:
  bool is_success_access_required = false;
};

} // namespace odu
} // namespace ocudu
