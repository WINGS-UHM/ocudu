/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "rrc_ue_helpers.h"
#include "ocudu/asn1/rrc_nr/dl_ccch_msg.h"
#include "ocudu/asn1/rrc_nr/dl_dcch_msg.h"
#include "ocudu/asn1/rrc_nr/ul_ccch_msg.h"
#include "ocudu/asn1/rrc_nr/ul_ccch_msg_ies.h"
#include "ocudu/asn1/rrc_nr/ul_dcch_msg.h"
#include "ocudu/asn1/rrc_nr/ul_dcch_msg_ies.h"

using namespace ocudu;
using namespace ocucp;

template <class T>
void ocudu::ocucp::log_rrc_message(rrc_ue_logger&    logger,
                                   const direction_t dir,
                                   byte_buffer_view  pdu,
                                   const T&          msg,
                                   srb_id_t          srb_id,
                                   const char*       msg_type)
{
  if (logger.get_basic_logger().debug.enabled()) {
    asn1::json_writer js;
    msg.to_json(js);
    logger.log_debug(pdu.begin(),
                     pdu.end(),
                     "{} {} {} {} ({} B)",
                     (dir == Rx) ? "Rx" : "Tx",
                     srb_id,
                     msg_type,
                     msg.msg.c1().type().to_string(),
                     pdu.length());
    logger.log_debug("Containerized {}: {}", msg.msg.c1().type().to_string(), js.to_string());
  } else if (logger.get_basic_logger().info.enabled()) {
    std::vector<uint8_t> bytes{pdu.begin(), pdu.end()};
    logger.log_info(pdu.begin(), pdu.end(), "{} {}", msg_type, msg.msg.c1().type().to_string());
  }
}

template void ocudu::ocucp::log_rrc_message<asn1::rrc_nr::ul_ccch_msg_s>(rrc_ue_logger&                     logger,
                                                                         const direction_t                  dir,
                                                                         byte_buffer_view                   pdu,
                                                                         const asn1::rrc_nr::ul_ccch_msg_s& msg,
                                                                         srb_id_t                           srb_id,
                                                                         const char*                        msg_type);

template void ocudu::ocucp::log_rrc_message<asn1::rrc_nr::ul_dcch_msg_s>(rrc_ue_logger&                     logger,
                                                                         const direction_t                  dir,
                                                                         byte_buffer_view                   pdu,
                                                                         const asn1::rrc_nr::ul_dcch_msg_s& msg,
                                                                         srb_id_t                           srb_id,
                                                                         const char*                        msg_type);

template void ocudu::ocucp::log_rrc_message<asn1::rrc_nr::dl_ccch_msg_s>(rrc_ue_logger&                     logger,
                                                                         const direction_t                  dir,
                                                                         byte_buffer_view                   pdu,
                                                                         const asn1::rrc_nr::dl_ccch_msg_s& msg,
                                                                         srb_id_t                           srb_id,
                                                                         const char*                        msg_type);

template void ocudu::ocucp::log_rrc_message<asn1::rrc_nr::dl_dcch_msg_s>(rrc_ue_logger&                     logger,
                                                                         const direction_t                  dir,
                                                                         byte_buffer_view                   pdu,
                                                                         const asn1::rrc_nr::dl_dcch_msg_s& msg,
                                                                         srb_id_t                           srb_id,
                                                                         const char*                        msg_type);

rrc_ue_capabilities_t ocudu::ocucp::get_capabilities(asn1::rrc_nr::ue_nr_cap_s& ue_capabilities, rrc_ue_logger& logger)
{
  rrc_ue_capabilities_t capabilities{};

  // Set RRC Inactive support.
  if (ue_capabilities.non_crit_ext_present and ue_capabilities.non_crit_ext.inactive_state_present) {
    capabilities.rrc_inactive_supported = true;
    logger.log_debug("RRC Inactive supported by UE");
  }

  return capabilities;
}

std::optional<rrc_ue_capabilities_t>
ocudu::ocucp::get_capabilities(asn1::rrc_nr::ue_cap_rat_container_list_l& capabilities_list, rrc_ue_logger& logger)
{
  // Parse UE capabilities list.
  for (const auto& asn1_ue_capability_rat_container : capabilities_list) {
    if (asn1_ue_capability_rat_container.rat_type != asn1::rrc_nr::rat_type_opts::options::nr) {
      logger.log_debug("Skipping non-NR RAT type in UE capabilities");
      continue;
    }

    asn1::rrc_nr::ue_nr_cap_s asn1_ue_nr_cap;
    {
      asn1::cbit_ref bref(asn1_ue_capability_rat_container.ue_cap_rat_container);
      if (asn1_ue_nr_cap.unpack(bref) != asn1::OCUDUASN_SUCCESS) {
        logger.log_error(asn1_ue_capability_rat_container.ue_cap_rat_container.begin(),
                         asn1_ue_capability_rat_container.ue_cap_rat_container.end(),
                         "Failed to unpack UE Capabilitiy RAT container PDU");
        return std::nullopt;
      }
    }

    return get_capabilities(asn1_ue_nr_cap, logger);
  }

  return rrc_ue_capabilities_t{};
}
