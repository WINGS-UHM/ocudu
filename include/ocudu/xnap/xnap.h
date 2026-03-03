// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#pragma once

#include "xnap_message_notifier.h"
#include "ocudu/cu_cp/cu_cp_types.h"
#include "ocudu/support/async/async_task.h"
#include "ocudu/xnap/xnap_handover.h"

namespace ocudu::ocucp {

struct xnap_message;

/// This interface is used to push XNAP messages to the XNAP interface.
class xnap_message_handler
{
public:
  virtual ~xnap_message_handler() = default;

  /// Handle the incoming XNAP message.
  virtual void handle_message(const xnap_message& msg) = 0;
};

/// XNAP interface for CU-CP to initiate the XN interface.
class xnap_connection_manager
{
public:
  virtual ~xnap_connection_manager() = default;

  /// \brief Trigger the initiation of the XN setup procedure.
  /// \returns true if the procedure completed successfully, false otherwise.
  virtual async_task<bool> handle_xn_setup_request_required() = 0;

  /// \brief Provide the SCTP association notifier after the SCTP association establishment.
  /// \param[in] tx_notifier_ The SCTP association notifier.
  virtual void set_tx_association_notifier(std::unique_ptr<xnap_message_notifier> tx_notifier_) = 0;
};

class xnap_control_message_handler
{
public:
  virtual ~xnap_control_message_handler() = default;

  /// \brief Initiates a Handover Preparation procedure TS 38.423 section 8.2.1.
  virtual async_task<xnap_handover_preparation_response>
  handle_handover_preparation_request(const xnap_handover_preparation_request& msg) = 0;
};

/// XNAP notifier to the CU-CP.
class xnap_cu_cp_notifier
{
public:
  virtual ~xnap_cu_cp_notifier() = default;

  /// \brief Get packed handover preparation message for inter-gNB handover.
  virtual byte_buffer on_handover_preparation_message_required(ue_index_t ue_index) = 0;

  /// \brief Notify about the reception of a new RRC Handover Command (TS 38.331 section 11.2.2).
  /// \param[in] ue_index The index of the UE.
  /// \param[in] command The RRC container containing the Handover Command.
  /// \returns True if the Handover command is valid and was successfully handled by the DU.
  virtual async_task<bool> on_new_rrc_handover_command(ue_index_t ue_index, byte_buffer command) = 0;
};

/// Combined entry point for the XNAP object.
class xnap_interface : public xnap_message_handler, public xnap_connection_manager, public xnap_control_message_handler
{
public:
  virtual ~xnap_interface() = default;
};

} // namespace ocudu::ocucp
