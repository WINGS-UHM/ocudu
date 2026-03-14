// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#include "xnap_sn_status_transfer_procedure.h"

using namespace ocudu;
using namespace ocudu::ocucp;
using namespace asn1::xnap;

xnap_sn_status_transfer_procedure::xnap_sn_status_transfer_procedure(
    protocol_transaction_event_source<asn1::xnap::sn_status_transfer_s>& sn_status_transfer_outcome_,
    xnap_ue_logger&                                                      logger_) :
  sn_status_transfer_outcome(sn_status_transfer_outcome_), logger(logger_)
{
}

void xnap_sn_status_transfer_procedure::operator()(coro_context<async_task<expected<cu_cp_status_transfer>>>& ctx)
{
  CORO_BEGIN(ctx);
  logger.log_debug("\"{}\" started...", name());
  transaction_sink.subscribe_to(sn_status_transfer_outcome, std::chrono::milliseconds{5000});

  CORO_AWAIT(transaction_sink);

  if (transaction_sink.timeout_expired()) {
    logger.log_warning("\"{}\" timed out after {}ms", name(), 5000);
    CORO_EARLY_RETURN(make_unexpected(default_error_t{}));
  }

  if (not transaction_sink.successful()) {
    logger.log_debug("\"{}\" failed", name());
    CORO_EARLY_RETURN(make_unexpected(default_error_t{}));
  }

  if (not fill_xnap_sn_status_transfer()) {
    logger.log_debug("\"{}\" failed", name());
    CORO_EARLY_RETURN(make_unexpected(default_error_t{}));
  }

  logger.log_debug("\"{}\" finished successfully", name());
  CORO_RETURN(sn_status_transfer);
}

bool xnap_sn_status_transfer_procedure::fill_xnap_sn_status_transfer()
{
  sn_status_transfer.ue_index = ue_index;

  const asn1::xnap::sn_status_transfer_s& sn_status_transfer_asn1 = transaction_sink.response();

  for (const asn1::xnap::drbs_subject_to_status_transfer_item_s& drb_item_asn1 :
       sn_status_transfer_asn1->drbs_subject_to_status_transfer_list) {
    cu_cp_drbs_subject_to_status_transfer_item drb_item;

    drb_item.drb_id = uint_to_drb_id(drb_item_asn1.drb_id);
    // Fill DL status
    if (drb_item_asn1.pdcp_status_transfer_dl.type() == drb_b_status_transfer_choice_c::types_opts::pdcp_sn_12bits) {
      drb_item.drb_status_dl.sn_size     = pdcp_sn_size::size12bits;
      drb_item.drb_status_dl.dl_count.sn = drb_item_asn1.pdcp_status_transfer_dl.pdcp_sn_12bits().count_value.pdcp_sn12;
      drb_item.drb_status_dl.dl_count.hfn =
          drb_item_asn1.pdcp_status_transfer_dl.pdcp_sn_12bits().count_value.hfn_pdcp_sn12;
    } else if (drb_item_asn1.pdcp_status_transfer_dl.type() ==
               drb_b_status_transfer_choice_c::types_opts::pdcp_sn_18bits) {
      drb_item.drb_status_dl.sn_size     = pdcp_sn_size::size18bits;
      drb_item.drb_status_dl.dl_count.sn = drb_item_asn1.pdcp_status_transfer_dl.pdcp_sn_18bits().count_value.pdcp_sn18;
      drb_item.drb_status_dl.dl_count.hfn =
          drb_item_asn1.pdcp_status_transfer_dl.pdcp_sn_18bits().count_value.hfn_pdcp_sn18;
    } else {
      return false;
    }
    // Fill UL status.
    if (drb_item_asn1.pdcp_status_transfer_ul.type() == drb_b_status_transfer_choice_c::types_opts::pdcp_sn_12bits) {
      drb_item.drb_status_ul.sn_size     = pdcp_sn_size::size12bits;
      drb_item.drb_status_ul.ul_count.sn = drb_item_asn1.pdcp_status_transfer_ul.pdcp_sn_12bits().count_value.pdcp_sn12;
      drb_item.drb_status_ul.ul_count.hfn =
          drb_item_asn1.pdcp_status_transfer_ul.pdcp_sn_12bits().count_value.hfn_pdcp_sn12;
    } else if (drb_item_asn1.pdcp_status_transfer_ul.type() ==
               drb_b_status_transfer_choice_c::types_opts::pdcp_sn_18bits) {
      drb_item.drb_status_ul.sn_size     = pdcp_sn_size::size18bits;
      drb_item.drb_status_ul.ul_count.sn = drb_item_asn1.pdcp_status_transfer_ul.pdcp_sn_18bits().count_value.pdcp_sn18;
      drb_item.drb_status_ul.ul_count.hfn =
          drb_item_asn1.pdcp_status_transfer_ul.pdcp_sn_18bits().count_value.hfn_pdcp_sn18;
    } else {
      return false;
    }
    sn_status_transfer.drbs_subject_to_status_transfer_list.emplace(drb_item.drb_id, drb_item);
  }
  return true;
}
