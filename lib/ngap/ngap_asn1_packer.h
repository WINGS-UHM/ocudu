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

#include "ocudu/adt/byte_buffer.h"
#include "ocudu/gateways/sctp_network_gateway.h"
#include "ocudu/ngap/ngap.h"
#include "ocudu/ocudulog/ocudulog.h"
#include "ocudu/pcap/dlt_pcap.h"
#include <cstdio>

namespace ocudu {
namespace ocucp {

struct ngap_message;

class ngap_asn1_packer : public ocucp::ngap_message_handler
{
public:
  ngap_asn1_packer(sctp_network_gateway_data_handler& gw,
                   ngap_message_notifier&             amf_notifier_,
                   ngap_message_handler&              ngap,
                   dlt_pcap&                          pcap_);

  void handle_packed_pdu(const byte_buffer& pdu);

  void handle_message(const ocucp::ngap_message& msg) override;

private:
  ocudulog::basic_logger&            logger;
  sctp_network_gateway_data_handler& gw;
  ngap_message_notifier&             amf_notifier;
  ngap_message_handler&              ngap;
  dlt_pcap&                          pcap;
};

} // namespace ocucp
} // namespace ocudu
