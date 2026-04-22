// UHM WINGS Fake Base Station Research

#include "lib/ngap/ngap_asn1_helpers.h"
#include "ocudu/asn1/asn1_utils.h"
#include "ocudu/asn1/ngap/common.h"
#include "ocudu/ngap/ngap_context.h"
#include "ocudu/ngap/ngap_message.h"
#include "ocudu/ocudulog/ocudulog.h"
#include "ocudu/ran/plmn_identity.h"
#include <cstdio>
#include <stdexcept>

using namespace ocudu;
using namespace ocudu::ocucp;

static byte_buffer build_ng_setup_request()
{
  ngap_context_t ngap_ctxt = {{411, 22},
                              "tstgnb01",
                              "AMF",
                              amf_index_t::min,
                              {{7, {{plmn_identity::test_value(), {{slice_service_type{1}}}}}}},
                              {},
                              256};

  ngap_message ngap_msg = {};
  ngap_msg.pdu.set_init_msg();
  ngap_msg.pdu.init_msg().load_info_obj(ASN1_NGAP_ID_NG_SETUP);
  fill_asn1_ng_setup_request(ngap_msg.pdu.init_msg().value.ng_setup_request(), ngap_ctxt);

  byte_buffer   packed_pdu{byte_buffer::fallback_allocation_tag{}};
  asn1::bit_ref bref(packed_pdu);
  if (ngap_msg.pdu.pack(bref) != asn1::OCUDUASN_SUCCESS) {
    throw std::runtime_error("Failed to pack NGSetupRequest");
  }

  return packed_pdu;
}

static void dump_hex(const byte_buffer& pdu)
{
  for (uint8_t byte : pdu) {
    std::printf("%02x", byte);
  }
  std::printf("\n");
}

int main()
{
  ocudulog::init();

  try {
    const byte_buffer ng_setup_request = build_ng_setup_request();
    std::printf("Packed NGSetupRequest (%zu bytes): ", static_cast<size_t>(ng_setup_request.length()));
    dump_hex(ng_setup_request);
  } catch (const std::exception& e) {
    std::fprintf(stderr, "ngap_listener error: %s\n", e.what());
    return 1;
  }

  return 0;
}
