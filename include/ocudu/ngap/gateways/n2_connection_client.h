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

#include "ocudu/ngap/ngap.h"

namespace ocudu {
namespace ocucp {

/// This interface notifies the receeption of new NGAP messages over the NGAP interface.
class ngap_rx_message_notifier
{
public:
  virtual ~ngap_rx_message_notifier() = default;

  /// \brief This callback is invoked on each received NGAP message.
  /// \param[in] msg The received NGAP message.
  virtual void on_new_message(const ngap_message& msg) = 0;
};

/// Handler of N2 connection between CU-CP and AMF.
class n2_connection_client
{
public:
  virtual ~n2_connection_client() = default;

  /// Establish a new TNL association with an AMF.
  ///
  /// \param cu_cp_rx_pdu_notifier Notifier that will be used to forward the NGAP PDUs sent by the AMF to the CU-CP.
  /// \return Notifier that the CU-CP can use to send NGAP Tx PDUs to the AMF it connected to.
  virtual std::unique_ptr<ngap_message_notifier>
  handle_cu_cp_connection_request(std::unique_ptr<ngap_rx_message_notifier> cu_cp_rx_pdu_notifier) = 0;
};

} // namespace ocucp
} // namespace ocudu
