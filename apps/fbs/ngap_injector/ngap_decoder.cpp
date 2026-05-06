// UHM WINGS Fake Base Station Research

#include "ngap_decoder.h"
#include "lib/ngap/ngap_asn1_helpers.h"
#include "lib/ngap/ngap_asn1_utils.h"
#include "ocudu/adt/span.h"
#include "ocudu/asn1/asn1_utils.h"
#include "ocudu/asn1/ngap/common.h"
#include "ocudu/asn1/ngap/ngap.h"
#include "ocudu/asn1/ngap/ngap_ies.h"
#include "ocudu/asn1/ngap/ngap_pdu_contents.h"
#include "ocudu/ngap/ngap_context.h"
#include "ocudu/ngap/ngap_message.h"
#include "ocudu/ran/plmn_identity.h"
#include <sstream>
#include <stdexcept>

using namespace ocudu;
using namespace ocudu::fbs;
using namespace ocudu::ocucp;

namespace {

byte_buffer make_byte_buffer_from_vector(const std::vector<uint8_t>& payload)
{
  byte_buffer buf{byte_buffer::fallback_allocation_tag{}};
  if (!payload.empty() && !buf.append(span<const uint8_t>(payload.data(), payload.size()))) {
    throw std::runtime_error("Failed to allocate NGAP payload buffer");
  }
  return buf;
}

std::string plmn_to_string(const asn1::fixed_octstring<3, true>& plmn)
{
  auto parsed = plmn_identity::from_bytes(plmn.to_bytes());
  return parsed.has_value() ? parsed.value().to_string() : plmn.to_string();
}

std::string guami_to_string(const asn1::ngap::guami_s& guami)
{
  std::ostringstream os;
  os << "plmn=" << plmn_to_string(guami.plmn_id) << " amf_region_id=" << guami.amf_region_id.to_number()
     << " amf_set_id=" << guami.amf_set_id.to_number() << " amf_pointer=" << guami.amf_pointer.to_number();
  return os.str();
}

void summarize_ng_setup_request(ngap_message_summary& summary, const asn1::ngap::ng_setup_request_s& msg)
{
  if (msg->ran_node_name_present) {
    summary.details.push_back(std::string("ran_node_name=") + msg->ran_node_name.to_string());
  }
  if (msg->global_ran_node_id.type().value == asn1::ngap::global_ran_node_id_c::types_opts::global_gnb_id) {
    const auto& global_gnb = msg->global_ran_node_id.global_gnb_id();
    summary.plmn           = plmn_to_string(global_gnb.plmn_id);
    if (global_gnb.gnb_id.type().value == asn1::ngap::gnb_id_c::types_opts::gnb_id) {
      summary.gnb_id = global_gnb.gnb_id.gnb_id().to_number();
    }
  }
  if (msg->supported_ta_list.size() != 0) {
    const auto& ta = msg->supported_ta_list[0];
    summary.tac    = ta.tac.to_number();
    if (ta.broadcast_plmn_list.size() != 0) {
      summary.plmn = plmn_to_string(ta.broadcast_plmn_list[0].plmn_id);
    }
  }
}

void summarize_ng_setup_response(ngap_message_summary& summary, const asn1::ngap::ng_setup_resp_s& msg)
{
  summary.details.push_back(std::string("amf_name=") + msg->amf_name.to_string());
  summary.details.push_back("relative_amf_capacity=" + std::to_string(msg->relative_amf_capacity));
  if (msg->served_guami_list.size() != 0) {
    summary.guami = guami_to_string(msg->served_guami_list[0].guami);
  }
  if (msg->plmn_support_list.size() != 0) {
    summary.plmn = plmn_to_string(msg->plmn_support_list[0].plmn_id);
  }
}

void summarize_init_message(ngap_message_summary& summary, const asn1::ngap::init_msg_s& init_msg)
{
  using init_types = asn1::ngap::ngap_elem_procs_o::init_msg_c::types_opts;

  // TODO: Extend IE-specific extraction as lab validation needs grow; replay keeps unknown IEs intact.
  switch (init_msg.value.type().value) {
    case init_types::ng_setup_request:
      summarize_ng_setup_request(summary, init_msg.value.ng_setup_request());
      break;
    case init_types::init_ue_msg:
      summary.details.push_back("nas_pdu_bytes=" + std::to_string(init_msg.value.init_ue_msg()->nas_pdu.size()));
      break;
    case init_types::dl_nas_transport:
      summary.details.push_back("nas_pdu_bytes=" + std::to_string(init_msg.value.dl_nas_transport()->nas_pdu.size()));
      break;
    case init_types::ul_nas_transport:
      summary.details.push_back("nas_pdu_bytes=" + std::to_string(init_msg.value.ul_nas_transport()->nas_pdu.size()));
      break;
    case init_types::ue_context_release_request:
      summary.cause = asn1_utils::get_cause_str(init_msg.value.ue_context_release_request()->cause);
      break;
    case init_types::ue_context_release_cmd:
      summary.cause = asn1_utils::get_cause_str(init_msg.value.ue_context_release_cmd()->cause);
      break;
    case init_types::error_ind:
      if (init_msg.value.error_ind()->cause_present) {
        summary.cause = asn1_utils::get_cause_str(init_msg.value.error_ind()->cause);
      }
      break;
    default:
      break;
  }
}

void summarize_successful_outcome(ngap_message_summary& summary, const asn1::ngap::successful_outcome_s& outcome)
{
  using success_types = asn1::ngap::ngap_elem_procs_o::successful_outcome_c::types_opts;

  switch (outcome.value.type().value) {
    case success_types::ng_setup_resp:
      summarize_ng_setup_response(summary, outcome.value.ng_setup_resp());
      break;
    default:
      break;
  }
}

void summarize_unsuccessful_outcome(ngap_message_summary&             summary,
                                    const asn1::ngap::unsuccessful_outcome_s& outcome)
{
  using unsuccess_types = asn1::ngap::ngap_elem_procs_o::unsuccessful_outcome_c::types_opts;

  switch (outcome.value.type().value) {
    case unsuccess_types::ng_setup_fail:
      summary.cause = asn1_utils::get_cause_str(outcome.value.ng_setup_fail()->cause);
      if (outcome.value.ng_setup_fail()->time_to_wait_present) {
        summary.details.push_back(std::string("time_to_wait=") +
                                  outcome.value.ng_setup_fail()->time_to_wait.to_string());
      }
      break;
    default:
      break;
  }
}

void fill_common_summary(ngap_message_summary& summary, const asn1::ngap::ngap_pdu_c& pdu)
{
  summary.message_type = asn1_utils::get_message_type_str(pdu);

  if (auto ran_id = asn1_utils::get_ran_ue_id(pdu)) {
    summary.ran_ue_ngap_id = ran_ue_id_to_uint(*ran_id);
  }
  if (auto amf_id = asn1_utils::get_amf_ue_id(pdu)) {
    summary.amf_ue_ngap_id = amf_ue_id_to_uint(*amf_id);
  }

  switch (pdu.type().value) {
    case asn1::ngap::ngap_pdu_c::types_opts::init_msg:
      summary.direction      = "initiating-message";
      summary.procedure_code = pdu.init_msg().proc_code;
      summary.criticality    = pdu.init_msg().crit.to_string();
      summarize_init_message(summary, pdu.init_msg());
      break;
    case asn1::ngap::ngap_pdu_c::types_opts::successful_outcome:
      summary.direction      = "successful-outcome";
      summary.procedure_code = pdu.successful_outcome().proc_code;
      summary.criticality    = pdu.successful_outcome().crit.to_string();
      summarize_successful_outcome(summary, pdu.successful_outcome());
      break;
    case asn1::ngap::ngap_pdu_c::types_opts::unsuccessful_outcome:
      summary.direction      = "unsuccessful-outcome";
      summary.procedure_code = pdu.unsuccessful_outcome().proc_code;
      summary.criticality    = pdu.unsuccessful_outcome().crit.to_string();
      summarize_unsuccessful_outcome(summary, pdu.unsuccessful_outcome());
      break;
    default:
      break;
  }
}

byte_buffer pack_message(const ngap_message& msg, const char* name)
{
  byte_buffer   packed_pdu{byte_buffer::fallback_allocation_tag{}};
  asn1::bit_ref bref(packed_pdu);
  if (msg.pdu.pack(bref) != asn1::OCUDUASN_SUCCESS) {
    throw std::runtime_error(std::string("Failed to pack ") + name);
  }
  return packed_pdu;
}

} // namespace

ngap_message_summary ocudu::fbs::decode_ngap_payload(const std::vector<uint8_t>& payload)
{
  return decode_ngap_payload(make_byte_buffer_from_vector(payload));
}

ngap_message_summary ocudu::fbs::decode_ngap_payload(const byte_buffer& payload)
{
  ngap_message_summary summary;
  summary.payload_length = payload.length();

  asn1::cbit_ref bref(payload);
  ngap_message    msg = {};
  if (msg.pdu.unpack(bref) != asn1::OCUDUASN_SUCCESS) {
    summary.decode_ok    = false;
    summary.decode_error = "NGAP ASN.1 unpack failed";
    return summary;
  }

  summary.decode_ok = true;
  fill_common_summary(summary, msg.pdu);
  return summary;
}

std::string ocudu::fbs::format_summary(const ngap_message_summary& summary)
{
  std::ostringstream os;
  os << "decode_ok=" << (summary.decode_ok ? "true" : "false") << " bytes=" << summary.payload_length;
  if (!summary.decode_ok) {
    os << " error=\"" << summary.decode_error << "\"";
    return os.str();
  }

  os << " direction=" << summary.direction << " message_type=" << summary.message_type
     << " procedure_code=" << summary.procedure_code << " criticality=" << summary.criticality;
  if (summary.ran_ue_ngap_id) {
    os << " ran_ue_ngap_id=" << *summary.ran_ue_ngap_id;
  }
  if (summary.amf_ue_ngap_id) {
    os << " amf_ue_ngap_id=" << *summary.amf_ue_ngap_id;
  }
  if (!summary.plmn.empty()) {
    os << " plmn=" << summary.plmn;
  }
  if (summary.tac) {
    os << " tac=" << *summary.tac;
  }
  if (summary.gnb_id) {
    os << " gnb_id=" << *summary.gnb_id;
  }
  if (!summary.guami.empty()) {
    os << " guami=\"" << summary.guami << "\"";
  }
  if (!summary.cause.empty()) {
    os << " cause=" << summary.cause;
  }
  for (const auto& detail : summary.details) {
    os << " " << detail;
  }
  return os.str();
}

bool ocudu::fbs::is_ng_setup_request(const ngap_message_summary& summary)
{
  return summary.decode_ok && summary.message_type == "NGSetupRequest";
}

bool ocudu::fbs::is_ng_setup_response(const ngap_message_summary& summary)
{
  return summary.decode_ok && summary.message_type == "NGSetupResponse";
}

bool ocudu::fbs::is_ng_setup_failure(const ngap_message_summary& summary)
{
  return summary.decode_ok && summary.message_type == "NGSetupFailure";
}

byte_buffer ocudu::fbs::build_ng_setup_request(const injector_config& cfg)
{
  auto plmn = plmn_identity::parse(cfg.gnb.plmn);
  if (!plmn.has_value()) {
    throw std::runtime_error("Invalid gNB PLMN in config: " + cfg.gnb.plmn);
  }
  if (cfg.gnb.id_bit_length < 22 || cfg.gnb.id_bit_length > 32) {
    throw std::runtime_error("gNB id_bit_length must be in the range [22, 32]");
  }

  ngap_context_t ngap_ctxt = {};
  ngap_ctxt.gnb_id         = {cfg.gnb.id, static_cast<uint8_t>(cfg.gnb.id_bit_length)};
  ngap_ctxt.ran_node_name  = cfg.gnb.ran_node_name;
  ngap_ctxt.amf_name       = "AMF";
  ngap_ctxt.amf_index      = amf_index_t::min;
  ngap_ctxt.supported_tas  = {{static_cast<tac_t>(cfg.gnb.tac),
                               {{plmn.value(), {{slice_service_type{cfg.gnb.sst}}}}}}};
  ngap_ctxt.default_paging_drx = cfg.gnb.default_paging_drx;

  ngap_message ngap_msg = {};
  ngap_msg.pdu.set_init_msg();
  ngap_msg.pdu.init_msg().load_info_obj(ASN1_NGAP_ID_NG_SETUP);
  fill_asn1_ng_setup_request(ngap_msg.pdu.init_msg().value.ng_setup_request(), ngap_ctxt);

  return pack_message(ngap_msg, "NGSetupRequest");
}

byte_buffer ocudu::fbs::build_ue_context_release_request(uint64_t amf_ue_ngap_id, uint64_t ran_ue_ngap_id)
{
  ngap_message ngap_msg = {};
  ngap_msg.pdu.set_init_msg();
  ngap_msg.pdu.init_msg().load_info_obj(ASN1_NGAP_ID_UE_CONTEXT_RELEASE_REQUEST);
  auto& request              = ngap_msg.pdu.init_msg().value.ue_context_release_request();
  request->amf_ue_ngap_id    = amf_ue_ngap_id;
  request->ran_ue_ngap_id    = ran_ue_ngap_id;
  request->cause.set_radio_network() = asn1::ngap::cause_radio_network_opts::options::user_inactivity;
  return pack_message(ngap_msg, "UEContextReleaseRequest");
}

byte_buffer ocudu::fbs::build_ng_reset_message()
{
  ngap_message ngap_msg = {};
  ngap_msg.pdu.set_init_msg();
  ngap_msg.pdu.init_msg().load_info_obj(ASN1_NGAP_ID_NG_RESET);
  auto& reset = ngap_msg.pdu.init_msg().value.ng_reset();
  reset->cause.set_radio_network() =
      asn1::ngap::cause_radio_network_opts::options::release_due_to_ngran_generated_reason;
  reset->reset_type.set_ng_interface() = asn1::ngap::reset_all_opts::options::reset_all;
  return pack_message(ngap_msg, "NGReset");
}

std::vector<uint8_t> ocudu::fbs::to_vector(const byte_buffer& payload)
{
  return std::vector<uint8_t>(payload.begin(), payload.end());
}
