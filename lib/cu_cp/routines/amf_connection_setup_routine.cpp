/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "amf_connection_setup_routine.h"
#include "ocudu/cu_cp/cu_cp_types.h"
#include "ocudu/ngap/ngap_setup.h"
#include "ocudu/support/async/coroutine.h"

using namespace ocudu;
using namespace ocucp;

async_task<bool>
ocudu::ocucp::start_amf_connection_setup(ngap_repository&                                    ngap_db,
                                         std::unordered_map<amf_index_t, std::atomic<bool>>& amfs_connected)
{
  return launch_async<amf_connection_setup_routine>(ngap_db, amfs_connected);
}

amf_connection_setup_routine::amf_connection_setup_routine(
    ngap_repository&                                    ngap_db_,
    std::unordered_map<amf_index_t, std::atomic<bool>>& amfs_connected_) :
  ngap_db(ngap_db_),
  amfs_connected(amfs_connected_),
  ngaps(ngap_db_.get_ngaps()),
  logger(ocudulog::fetch_basic_logger("CU-CP"))
{
}

void amf_connection_setup_routine::operator()(coro_context<async_task<bool>>& ctx)
{
  CORO_BEGIN(ctx);

  // TODO: Use when_all.
  for (ngap_it = ngaps.begin(); ngap_it != ngaps.end(); ngap_it++) {
    amf_index = ngap_it->first;
    ngap      = ngap_it->second;

    if (not ngap->handle_amf_tnl_connection_request()) {
      CORO_EARLY_RETURN(false);
    }

    // Initiate NG Setup.
    CORO_AWAIT_VALUE(result_msg, ngap->handle_ng_setup_request(/*max_setup_retries*/ 1));

    success = std::holds_alternative<ngap_ng_setup_response>(result_msg);

    // Handle result of NG setup.
    handle_connection_setup_result();

    if (success) {
      // Update PLMN lookups in NGAP repository after successful NGSetup.
      ngap_db.update_plmn_lookup(amf_index);

      std::string plmn_list;
      for (const auto& plmn : ngap->get_ngap_context().get_supported_plmns()) {
        plmn_list += plmn.to_string() + " ";
      }

      logger.info("Connected to AMF. Supported PLMNs: {}", plmn_list);
    } else {
      logger.error("Failed to connect to AMF");
      CORO_EARLY_RETURN(false);
    }
  }

  CORO_RETURN(success);
}

void amf_connection_setup_routine::handle_connection_setup_result()
{
  // Update AMF connection handler state.
  amfs_connected.emplace(amf_index, success);
}
