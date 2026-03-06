// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#include "xnc_connection_manager.h"
#include "ocudu/cu_cp/cu_cp_types.h"
#include "ocudu/xnap/xnap_message.h"
#include "ocudu/xnap/xnap_message_notifier.h"

using namespace ocudu;
using namespace ocucp;

/// Context of a XNC connection which is shared between the xnc_connection_manager and the xnap_message_notifier.
class xnc_connection_manager::shared_xnc_connection_context
{
public:
  shared_xnc_connection_context(xnc_connection_manager& parent_) : parent(parent_) {}
  shared_xnc_connection_context(const shared_xnc_connection_context&)            = delete;
  shared_xnc_connection_context(shared_xnc_connection_context&&)                 = delete;
  shared_xnc_connection_context& operator=(const shared_xnc_connection_context&) = delete;
  shared_xnc_connection_context& operator=(shared_xnc_connection_context&&)      = delete;
  ~shared_xnc_connection_context() { disconnect(); }

  /// Assign a XNC repository index to the context. This is called when the SCTP assocation is created and
  /// attached to an XNAP handler.
  void connect_xnc(xnc_peer_index_t xnc_idx_)
  {
    xnc_idx     = xnc_idx_;
    msg_handler = parent.xnaps.find_xnap(xnc_idx);
  }

  /// Determines whether an XNAP message handlerrepository has been attached to this association.
  bool connected() const { return msg_handler != nullptr; }

  /// Deletes the associated XNC repository, if it exists.
  void disconnect()
  {
    if (not connected()) {
      // XNC was never allocated or was already removed.
      return;
    }

    // TODO: Notify the CU-CP that the connection is closed.

    xnc_idx     = xnc_peer_index_t::invalid;
    msg_handler = nullptr;
  }

  /// Handle XNAP message coming from the SCTP GW.
  void handle_message(const xnap_message& msg)
  {
    if (not connected()) {
      parent.logger.warning("Discarding Rx XNAP message. Cause: CU-CP XN-C connection has been closed");
      return;
    }

    // Forward message.
    msg_handler->handle_message(msg);
  }

private:
  xnc_connection_manager& parent;
  xnc_peer_index_t        xnc_idx     = xnc_peer_index_t::invalid;
  xnap_message_handler*   msg_handler = nullptr;
};

/// Notifier used to forward Rx XNAP messages from the XN-C GW to CU-CP in a thread safe manner.
class xnc_connection_manager::xnc_gw_to_cu_cp_pdu_adapter final : public xnap_message_notifier
{
public:
  xnc_gw_to_cu_cp_pdu_adapter(xnc_connection_manager& parent_, std::shared_ptr<shared_xnc_connection_context> ctxt_) :
    parent(parent_), ctxt(std::move(ctxt_))
  {
  }

  ~xnc_gw_to_cu_cp_pdu_adapter() override
  {
    // Defer destruction of context to CU-CP execution context.
    // Note: We make a copy of the shared_ptr of the context to extend its lifetime to when the defer callback actually
    // gets executed.
    // Note: We don't use move because the defer may fail.
    while (not parent.cu_cp_exec.defer([ctxt_cpy = ctxt]() { ctxt_cpy->disconnect(); })) {
      parent.logger.error("Failed to schedule XNC CU-CP removal task. Retrying...");
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  }

  bool on_new_message(const xnap_message& msg) override
  {
    // Dispatch the XNAP Rx message handling to the CU-CP executor.
    while (not parent.cu_cp_exec.execute([this, msg]() { ctxt->handle_message(msg); })) {
      parent.logger.error("Failed to dispatch XNAP message to CU-CP. Retrying...");
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return true;
  }

private:
  xnc_connection_manager&                        parent;
  std::shared_ptr<shared_xnc_connection_context> ctxt;
};

xnc_connection_manager::xnc_connection_manager(xnap_repository&       xnaps_,
                                               task_executor&         cu_cp_exec_,
                                               common_task_scheduler& common_task_sched_) :
  xnaps(xnaps_),
  cu_cp_exec(cu_cp_exec_),
  common_task_sched(common_task_sched_),
  logger(ocudulog::fetch_basic_logger("CU-CP"))
{
}

void xnc_connection_manager::start()
{
  // Schedules setup routine to be executed in sequence with other CU-CP procedures.
  common_task_sched.schedule_async_task(
      launch_async([xn_it     = std::map<xnc_peer_index_t, xnap_interface*>::iterator{},
                    xnaps_map = xnaps.get_xnaps()](coro_context<async_task<void>>& ctx) mutable {
        CORO_BEGIN(ctx);

        // TODO try to connect to all neighbours.
        for (xn_it = xnaps_map.begin(); xn_it != xnaps_map.end(); ++xn_it) {
          CORO_AWAIT(xn_it->second->handle_xn_setup_request_required());
          // TODO: Handle setup failure
        }

        CORO_RETURN();
      }));
}

void xnc_connection_manager::connect_to_neighbours()
{
  std::map<xnc_peer_index_t, xnap_interface*> xn = xnaps.get_xnaps();
  for (const std::pair<const xnc_peer_index_t, xnap_interface*>& xnap_it : xn) {
    xnap_it.second->handle_xn_setup_request_required();
    // TODO: Handle setup failure
  }
}

void xnc_connection_manager::stop()
{
  if (stopped) {
    return;
  }

  // TODO: Disconnect from XNC neighbours and stop accepting new connections.

  stopped = true;
}

std::unique_ptr<xnap_message_notifier>
xnc_connection_manager::handle_new_xnc_cu_cp_connection(std::unique_ptr<xnap_message_notifier> xnap_tx_pdu_notifier,
                                                        const sctp_association_info&           assoc_info)
{
  // Note: This function may be called from a different execution context than the CU-CP.
  if (stopped.load(std::memory_order_acquire)) {
    // CU-CP is in the process of being stopped.
    return nullptr;
  }

  // We create a "detached" notifier, that has no associated XNAP yet.
  auto shared_ctxt     = std::make_shared<shared_xnc_connection_context>(*this);
  auto rx_pdu_notifier = std::make_unique<xnc_gw_to_cu_cp_pdu_adapter>(*this, shared_ctxt);

  // Find XNAP neighbour. This needs to be done over the CU-CP execution context, so
  // we dispatch the task to find the correct XNAP and "attach" it to the notifier
  while (not cu_cp_exec.execute(
      [this, shared_ctxt, sender_notifier = std::move(xnap_tx_pdu_notifier), addr = assoc_info.peer_addr]() mutable {
        // Find XNAP based on address of peer.
        xnc_peer_index_t xnc_index = xnaps.find_xnap(addr);
        if (xnc_index == xnc_peer_index_t::invalid) {
          logger.warning("Rejecting new CU-CP connection. Cause: Failed to create a new XNAP for peer address {}",
                         addr);
          return;
        }

        xnaps.connect_association(xnc_index, std::move(sender_notifier));

        // Register the XNAP peer in the shared XNAP connection context.
        shared_ctxt->connect_xnc(xnc_index);

        if (not xnc_connections.insert(std::make_pair(xnc_index, std::move(shared_ctxt))).second) {
          logger.error("Failed to store new CU-CP connection {}", fmt::underlying(xnc_index));
          return;
        }

        logger.debug("Added TNL association to XN-C {}", xnc_index);
      })) {
    logger.debug("Failed to dispatch CU-CP connection task. Retrying...");
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  return rx_pdu_notifier;
}
