// Copyright 2021-2026 Software Radio Systems Limited
// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#include "xnap_repository.h"
#include "ocudu/cu_cp/cu_cp_types.h"
#include "ocudu/support/ocudu_assert.h"
#include "ocudu/xnap/xnap_factory.h"
#include <algorithm>

using namespace ocudu;
using namespace ocucp;

xnap_repository::xnap_repository(xnap_repository_config cfg_) : cfg(cfg_), logger(cfg.logger) {}

std::map<xnc_peer_index_t, xnap_interface*> xnap_repository::get_xnaps()
{
  std::map<xnc_peer_index_t, xnap_interface*> xnaps;
  for (auto& peer : xnap_db) {
    xnaps.emplace(peer.first, peer.second.xnap.get());
  }
  return xnaps;
}

xnap_interface* xnap_repository::add_xnap(xnc_peer_index_t               xnc_index,
                                          const transport_layer_address& peer_addr,
                                          const xnap_configuration&      xnap_cfg)
{
  auto it = xnap_db.insert(std::make_pair(xnc_index, xnap_context{}));
  ocudu_assert(it.second, "Unable to insert XNAP in map");
  xnap_context& xnap_ctxt = it.first->second;
  xnap_ctxt.peer_addr     = peer_addr;
  // TODO connect XNAP handler to CU-CP.

  // Create XNAP object with initial Tx notifier. The notifier will be replaced with the one from the association once
  // the association is established.
  std::unique_ptr<xnap_interface> xnap_entity =
      create_xnap(xnap_cfg, cfg.cu_cp.xnap.xnc_gw->get_init_tx_notifier(peer_addr), *cfg.cu_cp.services.cu_cp_executor);
  if (xnap_entity == nullptr) {
    logger.error("Failed to create XNAP");
    xnap_db.erase(it.first);
    return nullptr;
  }

  xnap_ctxt.xnap = std::move(xnap_entity);

  return xnap_ctxt.xnap.get();
}

xnap_interface* xnap_repository::find_xnap(xnc_peer_index_t xnc_index)
{
  auto it = xnap_db.find(xnc_index);
  if (it == xnap_db.end()) {
    return nullptr;
  }
  return it->second.xnap.get();
}

xnc_peer_index_t xnap_repository::find_xnap(const transport_layer_address& peer_addr)
{
  auto it = std::find_if(
      xnap_db.begin(), xnap_db.end(), [&peer_addr](const std::pair<const xnc_peer_index_t, xnap_context>& xn) {
        return xn.second.peer_addr == peer_addr;
      });
  if (it == xnap_db.end()) {
    return xnc_peer_index_t::invalid;
  }
  return it->first;
}

void xnap_repository::connect_association(xnc_peer_index_t idx, std::unique_ptr<xnap_message_notifier> sender_notifier)
{
  auto it = xnap_db.find(idx);
  if (it == xnap_db.end()) {
    return;
  }
  xnap_context& ctx = it->second;
  ctx.xnap->set_tx_association_notifier(std::move(sender_notifier));
}
