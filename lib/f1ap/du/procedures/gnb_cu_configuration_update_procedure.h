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

#include "ocudu/asn1/f1ap/f1ap_pdu_contents.h"
#include "ocudu/f1ap/du/f1ap_du.h"
#include "ocudu/support/async/async_task.h"

namespace ocudu {

class f1ap_message_notifier;

namespace odu {

class f1ap_du_configurator;

class gnb_cu_configuration_update_procedure
{
public:
  gnb_cu_configuration_update_procedure(const asn1::f1ap::gnb_cu_cfg_upd_s& msg_,
                                        f1ap_du_configurator&               du_mng_,
                                        f1ap_message_notifier&              cu_notif_);

  void operator()(coro_context<async_task<void>>& ctx);

private:
  gnbcu_config_update_request request_du_update();

  void send_response();

  const asn1::f1ap::gnb_cu_cfg_upd_s request;
  f1ap_du_configurator&              du_mng;
  f1ap_message_notifier&             cu_notif;

  gnbcu_config_update_response du_resp;
};

} // namespace odu
} // namespace ocudu
