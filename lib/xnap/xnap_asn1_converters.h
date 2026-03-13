// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#pragma once

#include "ocudu/asn1/xnap/xnap_ies.h"
#include "ocudu/cu_cp/inter_cu_handover_messages.h"
#include "ocudu/ran/cause/xnap_cause.h"
#include "ocudu/ran/cu_types.h"
#include "ocudu/ran/guami.h"
#include "ocudu/ran/nr_cgi.h"
#include "ocudu/ran/qos/qos_parameters.h"
#include "ocudu/ran/s_nssai.h"
#include "ocudu/security/security_asn1_utils.h"

namespace ocudu::ocucp {

/// \brief Convert \c xnap_cause_t type to XNAP ASN.1 cause.
/// \param cause The xnap_cause_t type.
/// \return The XNAP cause.
inline asn1::xnap::cause_c cause_to_asn1(xnap_cause_t cause)
{
  asn1::xnap::cause_c asn1_cause;

  if (const auto* result = std::get_if<xnap_cause_radio_network_t>(&cause)) {
    asn1_cause.set_radio_network() = static_cast<asn1::xnap::cause_radio_network_layer_opts::options>(*result);
    return asn1_cause;
  }
  if (const auto* result = std::get_if<xnap_cause_transport_t>(&cause)) {
    asn1_cause.set_transport() = static_cast<asn1::xnap::cause_transport_layer_opts::options>(*result);
    return asn1_cause;
  }

  if (const auto* result = std::get_if<cause_protocol_t>(&cause)) {
    asn1_cause.set_protocol() = static_cast<asn1::xnap::cause_protocol_opts::options>(*result);
    return asn1_cause;
  }
  if (const auto* result = std::get_if<xnap_cause_misc_t>(&cause)) {
    asn1_cause.set_misc() = static_cast<asn1::xnap::cause_misc_opts::options>(*result);
    return asn1_cause;
  }

  report_fatal_error("Cannot convert cause to XNAP type:{}", cause);
  return asn1_cause;
}

/// \brief Convert XNAP ASN.1 cause to \c xnap_cause_t type.
/// \param asn1_cause The XNAP ASN.1 cause.
/// \return The \c xnap_cause_t type.
inline xnap_cause_t asn1_to_cause(const asn1::xnap::cause_c& asn1_cause)
{
  xnap_cause_t cause;

  switch (asn1_cause.type()) {
    case asn1::xnap::cause_c::types_opts::radio_network:
      cause = static_cast<xnap_cause_radio_network_t>(asn1_cause.radio_network().value);
      break;
    case asn1::xnap::cause_c::types_opts::transport:
      cause = static_cast<xnap_cause_transport_t>(asn1_cause.transport().value);
      break;
    case asn1::xnap::cause_c::types_opts::protocol:
      cause = static_cast<cause_protocol_t>(asn1_cause.protocol().value);
      break;
    case asn1::xnap::cause_c::types_opts::misc:
      cause = static_cast<xnap_cause_misc_t>(asn1_cause.misc().value);
      break;
    default:
      report_fatal_error("Cannot convert XNAP ASN.1 cause {} to common type", fmt::underlying(asn1_cause.type().value));
  }

  return cause;
}

/// \brief Converts internal CGI struct into ASN.1 encoded NR-CGI.
/// \param[in] cgi The internal CGI struct.
/// \return The ASN.1 encoded NR-CGI.
inline asn1::xnap::nr_cgi_s cgi_to_asn1(const nr_cell_global_id_t& cgi)
{
  asn1::xnap::nr_cgi_s asn1_nr_cgi;
  asn1_nr_cgi.nr_ci.from_number(cgi.nci.value());
  asn1_nr_cgi.plmn_id = cgi.plmn_id.to_bytes();
  return asn1_nr_cgi;
}

/// \brief Converts ASN.1 encoded NR-CGI into internal CGI struct.
/// \param[in] asn1_cgi The ASN.1 encoded NR-CGI.
/// \return The internal CGI struct.
inline nr_cell_global_id_t asn1_to_cgi(const asn1::xnap::nr_cgi_s& asn1_cgi)
{
  nr_cell_global_id_t cgi = {};
  cgi.nci                 = nr_cell_identity::create(asn1_cgi.nr_ci.to_number()).value();
  cgi.plmn_id             = plmn_identity::from_bytes(asn1_cgi.plmn_id.to_bytes()).value();
  return cgi;
}

/// \brief Converts internal GUAMI struct into ASN.1 encoded GUAMI.
/// \param[in] guami The internal GUAMI struct.
/// \return The ASN.1 encoded GUAMI.
inline asn1::xnap::guami_s guami_to_asn1(const guami_t& guami)
{
  asn1::xnap::guami_s asn1_guami;
  asn1_guami.plmn_id = guami.plmn.to_bytes();
  asn1_guami.amf_region_id.from_number(guami.amf_region_id);
  asn1_guami.amf_set_id.from_number(guami.amf_set_id);
  asn1_guami.amf_pointer.from_number(guami.amf_pointer);

  return asn1_guami;
}

/// \brief Converts ASN.1 encoded GUAMI into internal GUAMI struct.
/// \param[in] asn1_guami The ASN.1 encoded GUAMI.
/// \return The internal GUAMI struct.
inline guami_t asn1_to_guami(const asn1::xnap::guami_s& asn1_guami)
{
  guami_t guami;
  guami.plmn.from_bytes(asn1_guami.plmn_id.to_bytes());
  guami.amf_region_id = asn1_guami.amf_region_id.to_number();
  guami.amf_set_id    = asn1_guami.amf_set_id.to_number();
  guami.amf_pointer   = asn1_guami.amf_pointer.to_number();

  return guami;
}

/// \brief Convert XNAP ASN.1 security context to common type.
/// \param[out] sec_ctxt The common type security context.
/// \param[in] asn1_sec_cap The ASN.1 UE security capabilities.
/// \param[in] asn1_sec_info The ASN.1 AS security information.
/// \return True on success, otherwise false.
inline bool asn1_to_security_context(security::security_context&           sec_ctxt,
                                     const asn1::xnap::ue_security_cap_s&  asn1_sec_cap,
                                     const asn1::xnap::as_security_info_s& asn1_sec_info)
{
  security::asn1_to_key(sec_ctxt.k, asn1_sec_info.key_ng_ran_star);
  sec_ctxt.ncc = asn1_sec_info.ncc;
  security::asn1_to_supported_algorithms(sec_ctxt.supported_int_algos, asn1_sec_cap.nr_integrity_protection_algorithms);
  security::asn1_to_supported_algorithms(sec_ctxt.supported_enc_algos, asn1_sec_cap.nr_encyption_algorithms);
  ocudulog::fetch_basic_logger("XNAP").debug(asn1_sec_info.key_ng_ran_star.data(), 32, "K_gnb");
  ocudulog::fetch_basic_logger("XNAP").debug("Supported integrity algorithms: {}", sec_ctxt.supported_int_algos);
  ocudulog::fetch_basic_logger("XNAP").debug("Supported ciphering algorithms: {}", sec_ctxt.supported_enc_algos);

  return true;
}

/// \brief Converts common S-NSSAI type to ASN.1.
/// \param[out] s_nssai Common type S-NSSAI.
/// \return ASN.1 S-NSSAI type.
inline asn1::xnap::s_nssai_s s_nssai_to_asn1(const s_nssai_t& s_nssai)
{
  asn1::xnap::s_nssai_s asn1_s_nssai;
  asn1_s_nssai.sst.from_number(s_nssai.sst.value());

  if (s_nssai.sd.is_set()) {
    asn1_s_nssai.sd_present = true;
    asn1_s_nssai.sd.from_number(s_nssai.sd.value());
  }

  return asn1_s_nssai;
}

/// \brief Converts ASN.1 S-NSSAI type to common type.
/// \param[in] asn1_s_nssai The ASN.1 S-NSSAI type.
/// \return The common S-NSSAI type where the result of the conversion is stored.
inline s_nssai_t asn1_to_s_nssai(const asn1::xnap::s_nssai_s& asn1_s_nssai)
{
  s_nssai_t s_nssai;
  s_nssai.sst = slice_service_type{(uint8_t)asn1_s_nssai.sst.to_number()};
  if (asn1_s_nssai.sd_present) {
    s_nssai.sd = slice_differentiator::create(asn1_s_nssai.sd.to_number()).value();
  }

  return s_nssai;
}

inline asn1::xnap::gbr_qos_flow_info_s gbr_qos_flow_information_to_asn1(const gbr_qos_flow_information& gbr_qos_info)
{
  asn1::xnap::gbr_qos_flow_info_s asn1_gbr_qos_flow_info;

  asn1_gbr_qos_flow_info.max_flow_bit_rate_dl        = gbr_qos_info.max_br_dl;
  asn1_gbr_qos_flow_info.max_flow_bit_rate_ul        = gbr_qos_info.max_br_ul;
  asn1_gbr_qos_flow_info.guaranteed_flow_bit_rate_dl = gbr_qos_info.gbr_dl;
  asn1_gbr_qos_flow_info.guaranteed_flow_bit_rate_ul = gbr_qos_info.gbr_ul;
  if (gbr_qos_info.max_packet_loss_rate_dl.has_value()) {
    asn1_gbr_qos_flow_info.max_packet_loss_rate_dl_present = true;
    asn1_gbr_qos_flow_info.max_packet_loss_rate_dl         = gbr_qos_info.max_packet_loss_rate_dl.value();
  }
  if (gbr_qos_info.max_packet_loss_rate_ul.has_value()) {
    asn1_gbr_qos_flow_info.max_packet_loss_rate_ul_present = true;
    asn1_gbr_qos_flow_info.max_packet_loss_rate_ul         = gbr_qos_info.max_packet_loss_rate_ul.value();
  }

  return asn1_gbr_qos_flow_info;
}

/// \brief Converts common QoS flow level QoS parameters to ASN.1.
/// \param[in] qos_flow_level_params Common type QoS flow level QoS parameters.
/// \return ASN.1 QoS flow level QoS parameters.
inline asn1::xnap::qos_flow_level_qos_params_s
qos_flow_level_qos_parameters_to_asn1(const qos_flow_level_qos_parameters& qos_flow_level_params)
{
  asn1::xnap::qos_flow_level_qos_params_s asn1_qos_flow_level_params;

  // Fill QoS characteristics
  // > Fill dynamic 5QI.
  if (qos_flow_level_params.qos_desc.is_dyn_5qi()) {
    const auto& dynamic_5qi = qos_flow_level_params.qos_desc.get_dyn_5qi();
    asn1_qos_flow_level_params.qos_characteristics.set_dyn();
    auto& asn1_dynamic_5qi = asn1_qos_flow_level_params.qos_characteristics.dyn();

    asn1_dynamic_5qi.prio_level_qos                 = dynamic_5qi.qos_prio_level.value();
    asn1_dynamic_5qi.packet_delay_budget            = dynamic_5qi.packet_delay_budget;
    asn1_dynamic_5qi.packet_error_rate.per_scalar   = dynamic_5qi.per.scalar;
    asn1_dynamic_5qi.packet_error_rate.per_exponent = dynamic_5qi.per.exponent;
    if (dynamic_5qi.five_qi.has_value()) {
      asn1_dynamic_5qi.five_qi_present = true;
      asn1_dynamic_5qi.five_qi         = five_qi_to_uint(dynamic_5qi.five_qi.value());
    }
    if (dynamic_5qi.is_delay_critical.has_value()) {
      asn1_dynamic_5qi.delay_crit_present = true;
      asn1_dynamic_5qi.delay_crit.value   = dynamic_5qi.is_delay_critical.value()
                                                ? asn1::xnap::dyn_5qi_descriptor_s::delay_crit_opts::delay_crit
                                                : asn1::xnap::dyn_5qi_descriptor_s::delay_crit_opts::non_delay_crit;
    }
    if (dynamic_5qi.averaging_win.has_value()) {
      asn1_dynamic_5qi.averaging_win_present = true;
      asn1_dynamic_5qi.averaging_win         = dynamic_5qi.averaging_win.value();
    }
    if (dynamic_5qi.max_data_burst_volume.has_value()) {
      asn1_dynamic_5qi.max_data_burst_volume_present = true;
      asn1_dynamic_5qi.max_data_burst_volume         = dynamic_5qi.max_data_burst_volume.value();
    }
  } else /* Fill non dynamic 5QI. */ {
    const auto& non_dynamic_5qi = qos_flow_level_params.qos_desc.get_nondyn_5qi();
    asn1_qos_flow_level_params.qos_characteristics.set_non_dyn();
    auto& asn1_non_dynamic_5qi = asn1_qos_flow_level_params.qos_characteristics.non_dyn();

    asn1_non_dynamic_5qi.five_qi = five_qi_to_uint(non_dynamic_5qi.five_qi);

    if (non_dynamic_5qi.qos_prio_level.has_value()) {
      asn1_non_dynamic_5qi.prio_level_qos_present = true;
      asn1_non_dynamic_5qi.prio_level_qos         = non_dynamic_5qi.qos_prio_level.value().value();
    }
    if (non_dynamic_5qi.averaging_win.has_value()) {
      asn1_non_dynamic_5qi.averaging_win_present = true;
      asn1_non_dynamic_5qi.averaging_win         = non_dynamic_5qi.averaging_win.value();
    }
    if (non_dynamic_5qi.max_data_burst_volume.has_value()) {
      asn1_non_dynamic_5qi.max_data_burst_volume_present = true;
      asn1_non_dynamic_5qi.max_data_burst_volume         = non_dynamic_5qi.max_data_burst_volume.value();
    }
  }

  // Fill alloc and retention prio.
  asn1_qos_flow_level_params.alloc_and_retention_prio.prio_level =
      qos_flow_level_params.alloc_retention_prio.prio_level_arp.value();
  asn1_qos_flow_level_params.alloc_and_retention_prio.pre_emption_cap.value =
      qos_flow_level_params.alloc_retention_prio.may_trigger_preemption
          ? asn1::xnap::allocand_retention_prio_s::pre_emption_cap_opts::options::may_trigger_preemption
          : asn1::xnap::allocand_retention_prio_s::pre_emption_cap_opts::options::shall_not_trigger_preemption;
  asn1_qos_flow_level_params.alloc_and_retention_prio.pre_emption_vulnerability.value =
      qos_flow_level_params.alloc_retention_prio.is_preemptable
          ? asn1::xnap::allocand_retention_prio_s::pre_emption_vulnerability_opts::options::preemptable
          : asn1::xnap::allocand_retention_prio_s::pre_emption_vulnerability_opts::options::not_preemptable;

  // Fill GBR QoS flow info.
  if (qos_flow_level_params.gbr_qos_info.has_value()) {
    asn1_qos_flow_level_params.gbr_qos_flow_info_present = true;
    asn1_qos_flow_level_params.gbr_qos_flow_info =
        gbr_qos_flow_information_to_asn1(qos_flow_level_params.gbr_qos_info.value());
  }

  // Fill reflective QoS.
  if (qos_flow_level_params.reflective_qos_attribute_subject_to) {
    asn1_qos_flow_level_params.reflective_qos_present = true;
    asn1_qos_flow_level_params.reflective_qos =
        asn1::xnap::reflective_qos_attribute_opts::options::subject_to_reflective_qos;
  }

  // TODO: Fill missing optional parameters.

  return asn1_qos_flow_level_params;
}

/// \brief Convert XNAP ASN.1 to \c gbr_qos_flow_information.
/// \param[in] asn1_gbr_qos_info The ASN.1 type gbr qos info.
/// \return The common type gbr qos flow information.
inline gbr_qos_flow_information
asn1_to_gbr_qos_flow_information(const asn1::xnap::gbr_qos_flow_info_s& asn1_gbr_qos_info)
{
  gbr_qos_flow_information gbr_qos_info;
  gbr_qos_info.max_br_dl = asn1_gbr_qos_info.max_flow_bit_rate_dl;
  gbr_qos_info.max_br_ul = asn1_gbr_qos_info.max_flow_bit_rate_ul;
  gbr_qos_info.gbr_dl    = asn1_gbr_qos_info.guaranteed_flow_bit_rate_dl;
  gbr_qos_info.gbr_ul    = asn1_gbr_qos_info.guaranteed_flow_bit_rate_ul;
  if (asn1_gbr_qos_info.max_packet_loss_rate_dl_present) {
    gbr_qos_info.max_packet_loss_rate_dl = asn1_gbr_qos_info.max_packet_loss_rate_dl;
  }
  if (asn1_gbr_qos_info.max_packet_loss_rate_ul_present) {
    gbr_qos_info.max_packet_loss_rate_ul = asn1_gbr_qos_info.max_packet_loss_rate_ul;
  }

  return gbr_qos_info;
}

/// \brief Converts ASN.1 QoS flow level QoS parameters to common type.
/// \param[in] asn1_qos_flow_level_qos_params The ASN.1 QoS flow level QoS parameters.
/// \return The common type QoS flow level QoS parameters where the result of the conversion is stored.
inline qos_flow_level_qos_parameters
asn1_to_qos_flow_level_qos_parameters(const asn1::xnap::qos_flow_level_qos_params_s& asn1_qos_flow_level_qos_params)
{
  qos_flow_level_qos_parameters qos_flow_level_params;

  // Fill QoS flow level QoS parameters.
  if (asn1_qos_flow_level_qos_params.qos_characteristics.type() == asn1::xnap::qos_characteristics_c::types::dyn) {
    dyn_5qi_descriptor dyn_5qi = {};
    if (asn1_qos_flow_level_qos_params.qos_characteristics.dyn().five_qi_present) {
      dyn_5qi.five_qi = uint_to_five_qi(asn1_qos_flow_level_qos_params.qos_characteristics.dyn().five_qi);
    }
    // TODO: Add optional values.

    qos_flow_level_params.qos_desc = dyn_5qi;

    // TODO: Add optional values.

  } else if (asn1_qos_flow_level_qos_params.qos_characteristics.type() ==
             asn1::xnap::qos_characteristics_c::types::non_dyn) {
    non_dyn_5qi_descriptor non_dyn_5qi = {};
    non_dyn_5qi.five_qi = uint_to_five_qi(asn1_qos_flow_level_qos_params.qos_characteristics.non_dyn().five_qi);
    qos_flow_level_params.qos_desc = non_dyn_5qi;

    // TODO: Add optional values.
  }

  // Fill allocation and retention priority.
  qos_flow_level_params.alloc_retention_prio.prio_level_arp =
      asn1_qos_flow_level_qos_params.alloc_and_retention_prio.prio_level;
  qos_flow_level_params.alloc_retention_prio.may_trigger_preemption =
      asn1_qos_flow_level_qos_params.alloc_and_retention_prio.pre_emption_cap ==
      asn1::xnap::allocand_retention_prio_s::pre_emption_cap_opts::may_trigger_preemption;
  qos_flow_level_params.alloc_retention_prio.is_preemptable =
      asn1_qos_flow_level_qos_params.alloc_and_retention_prio.pre_emption_vulnerability ==
      asn1::xnap::allocand_retention_prio_s::pre_emption_vulnerability_opts::preemptable;

  // Fill GBR QoS flow info.
  if (asn1_qos_flow_level_qos_params.gbr_qos_flow_info_present) {
    qos_flow_level_params.gbr_qos_info =
        asn1_to_gbr_qos_flow_information(asn1_qos_flow_level_qos_params.gbr_qos_flow_info);
  }

  // Fill reflective QoS.
  if (asn1_qos_flow_level_qos_params.reflective_qos_present) {
    qos_flow_level_params.reflective_qos_attribute_subject_to = true;
  }

  return qos_flow_level_params;
}

/// \brief Converts common type PDU session type to ASN.1.
/// \param[in] pdu_session_type The common type PDU session type.
/// \return The ASN.1 PDU session type.
inline asn1::xnap::pdu_session_type_e pdu_session_type_to_asn1(const pdu_session_type_t& pdu_session_type)
{
  switch (pdu_session_type) {
    case pdu_session_type_t::ipv4:
      return asn1::xnap::pdu_session_type_e::ipv4;
    case pdu_session_type_t::ipv6:
      return asn1::xnap::pdu_session_type_e::ipv6;
    case pdu_session_type_t::ipv4v6:
      return asn1::xnap::pdu_session_type_e::ipv4v6;
    default:
      return asn1::xnap::pdu_session_type_e::ethernet;
  }
}

/// \brief Converts ASN.1 PDU session type to common type.
/// \param[in] ans1_pdu_session_type The ASN.1 PDU session type.
/// \return The common type PDU session type where the result of the conversion is stored.
inline pdu_session_type_t asn1_to_pdu_session_type(const asn1::xnap::pdu_session_type_e& ans1_pdu_session_type)
{
  switch (ans1_pdu_session_type) {
    case asn1::xnap::pdu_session_type_e::ipv4:
      return pdu_session_type_t::ipv4;
    case asn1::xnap::pdu_session_type_e::ipv6:
      return pdu_session_type_t::ipv6;
    case asn1::xnap::pdu_session_type_e::ipv4v6:
      return pdu_session_type_t::ipv4v6;
    default:
      return pdu_session_type_t::ethernet;
  }
}

/// \brief Converts type \c security_indication to an ASN.1 type.
/// \param[out] asn1obj ASN.1 object where the result of the conversion is stored.
/// \param[in] security_indication Security Indication IE contents.
inline void asn1_to_security_indication(security_indication_t& security_ind, const asn1::xnap::security_ind_s& asn1obj)
{
  switch (asn1obj.integrity_protection_ind) {
    case asn1::xnap::security_ind_s::integrity_protection_ind_opts::not_needed:
    case asn1::xnap::security_ind_s::integrity_protection_ind_opts::preferred:
    case asn1::xnap::security_ind_s::integrity_protection_ind_opts::required:
      security_ind.integrity_protection_ind =
          static_cast<integrity_protection_indication_t>(asn1obj.integrity_protection_ind.value);
      break;
    default:
      ocudulog::fetch_basic_logger("XNAP").error("Cannot convert security indication to XNAP type");
  }

  switch (asn1obj.confidentiality_protection_ind) {
    case asn1::xnap::security_ind_s::confidentiality_protection_ind_opts::not_needed:
    case asn1::xnap::security_ind_s::confidentiality_protection_ind_opts::preferred:
    case asn1::xnap::security_ind_s::confidentiality_protection_ind_opts::required:
      security_ind.confidentiality_protection_ind =
          static_cast<confidentiality_protection_indication_t>(asn1obj.confidentiality_protection_ind.value);
      break;
    default:
      ocudulog::fetch_basic_logger("XNAP").error("Cannot convert security indication to XNAP type");
  }
}

/// \brief Converts type \c up_transport_layer_info to an ASN.1 type.
/// \param asn1_up_tp_layer_info ASN.1 object where the result of the conversion is stored.
/// \param up_tp_layer_info UP Transport Layer Info object.
inline void up_transport_layer_info_to_asn1(asn1::xnap::up_transport_layer_info_c& asn1_up_tp_layer_info,
                                            const up_transport_layer_info&         up_tp_layer_info)
{
  asn1_up_tp_layer_info.set_gtp_tunnel();
  asn1_up_tp_layer_info.gtp_tunnel().gtp_teid.from_number(up_tp_layer_info.gtp_teid.value());
  asn1_up_tp_layer_info.gtp_tunnel().tnl_address.from_string(up_tp_layer_info.tp_address.to_bitstring());
}

/// \brief Convert common type PDU session resource admitted item to ASN.1.
/// \param[out] asn1_admitted_item The ASN.1 PDU session resource admitted item.
/// \param[in] admitted_item The common type PDU session resource admitted item.
/// \return True on success, otherwise false.
inline bool pdu_session_res_admitted_item_to_asn1(asn1::xnap::pdu_session_res_admitted_item_s&  asn1_admitted_item,
                                                  const cu_cp_xn_pdu_session_res_admitted_item& admitted_item)
{
  // Fill PDU session ID.
  asn1_admitted_item.pdu_session_id = pdu_session_id_to_uint(admitted_item.pdu_session_id);

  // Fill PDU session res admitted info.
  // > Fill DL NGU UP TNL info unchanged.
  if (admitted_item.dl_ngu_tnl_info_unchanged) {
    asn1_admitted_item.pdu_session_res_admitted_info.dl_ng_u_tnl_info_unchanged_present = true;
    asn1_admitted_item.pdu_session_res_admitted_info.dl_ng_u_tnl_info_unchanged =
        asn1::xnap::pdu_session_res_admitted_info_s::dl_ng_u_tnl_info_unchanged_opts::options::true_value;
  }
  // > Fill QoS flows admitted list.
  for (const auto& qos_flow_item : admitted_item.qos_flows_setup_list) {
    asn1::xnap::qos_flows_admitted_item_s asn1_qos_flow_admitted_item;
    // Fill QoS flow ID.
    asn1_qos_flow_admitted_item.qfi = qos_flow_id_to_uint(qos_flow_item.qos_flow_id);
    asn1_admitted_item.pdu_session_res_admitted_info.qos_flows_admitted_list.push_back(asn1_qos_flow_admitted_item);
  }
  // > Fill QoS flows not admitted list.
  for (const auto& qos_flow_not_admitted_item : admitted_item.qos_flows_failed_to_setup_list) {
    asn1::xnap::qos_flowwith_cause_item_s asn1_qos_flow_not_admitted_item;
    // Fill QoS flow ID.
    asn1_qos_flow_not_admitted_item.qfi = qos_flow_id_to_uint(qos_flow_not_admitted_item.qos_flow_id);
    // Fill cause.
    const auto* cause = std::get_if<std::optional<xnap_cause_t>>(&qos_flow_not_admitted_item.cause);
    if (cause && cause->has_value()) {
      asn1_qos_flow_not_admitted_item.cause_present = true;
      asn1_qos_flow_not_admitted_item.cause         = cause_to_asn1(cause->value());
    }

    asn1_admitted_item.pdu_session_res_admitted_info.qos_flows_not_admitted_list.push_back(
        asn1_qos_flow_not_admitted_item);
  }
  // > Fill data forwarding info from target.
  const auto& data_forwarding_info_from_target = admitted_item.data_forwarding_info_from_target;
  // >> Fill QoS flows accepted for data forwarding list.
  for (const auto& qfi : data_forwarding_info_from_target->qos_flows_accepted_for_data_forwarding_list) {
    asn1::xnap::qos_f_lows_accepted_to_be_forwarded_item_s asn1_qos_flow_accepted_for_data_forwarding_item;
    // Fill QoS flow ID.
    asn1_qos_flow_accepted_for_data_forwarding_item.qos_flow_id = qos_flow_id_to_uint(qfi);
    asn1_admitted_item.pdu_session_res_admitted_info.data_forwarding_info_from_target
        .qos_flows_accepted_for_data_forwarding_list.push_back(asn1_qos_flow_accepted_for_data_forwarding_item);
  }
  // >> Fill PDU session level DL data forwarding info.
  if (data_forwarding_info_from_target->pdu_session_level_dl_data_forwarding_info.has_value()) {
    asn1_admitted_item.pdu_session_res_admitted_info.data_forwarding_info_from_target
        .pdu_session_level_dl_data_forwarding_info_present = true;
    up_transport_layer_info_to_asn1(
        asn1_admitted_item.pdu_session_res_admitted_info.data_forwarding_info_from_target
            .pdu_session_level_dl_data_forwarding_info,
        data_forwarding_info_from_target->pdu_session_level_dl_data_forwarding_info.value());
  }
  // >> Fill PDU session level UL data forwarding info.
  if (data_forwarding_info_from_target->pdu_session_level_ul_data_forwarding_info.has_value()) {
    asn1_admitted_item.pdu_session_res_admitted_info.data_forwarding_info_from_target
        .pdu_session_level_ul_data_forwarding_info_present = true;
    up_transport_layer_info_to_asn1(
        asn1_admitted_item.pdu_session_res_admitted_info.data_forwarding_info_from_target
            .pdu_session_level_ul_data_forwarding_info,
        data_forwarding_info_from_target->pdu_session_level_ul_data_forwarding_info.value());
  }
  // >> Fill data forwarding resp DRB item list.
  for (const auto& drb_item : data_forwarding_info_from_target->data_forwarding_resp_drb_item_list) {
    asn1::xnap::data_forwarding_resp_drb_item_s asn1_drb_item;
    // Fill DRB ID.
    asn1_drb_item.drb_id = drb_id_to_uint(drb_item.drb_id);
    // Fill DL forwarding UP TNL.
    if (drb_item.dl_forwarding_up_tnl.has_value()) {
      asn1_drb_item.dl_forwarding_up_tnl_present = true;
      up_transport_layer_info_to_asn1(asn1_drb_item.dl_forwarding_up_tnl, drb_item.dl_forwarding_up_tnl.value());
    }
    // Fill UL forwarding UP TNL.
    if (drb_item.ul_forwarding_up_tnl.has_value()) {
      asn1_drb_item.ul_forwarding_up_tnl_present = true;
      up_transport_layer_info_to_asn1(asn1_drb_item.ul_forwarding_up_tnl, drb_item.ul_forwarding_up_tnl.value());
    }
    asn1_admitted_item.pdu_session_res_admitted_info.data_forwarding_info_from_target.data_forwarding_resp_drb_item_list
        .push_back(asn1_drb_item);
  }

  return true;
}

/// \brief Convert XNAP ASN.1 to \c cu_cp_tx_bw.
/// \param[in] asn1_tx_bw The ASN.1 type TX BW.
/// \return The common type TX BW.
inline cu_cp_tx_bw asn1_to_tx_bw(const asn1::xnap::nr_tx_bw_s& asn1_tx_bw)
{
  cu_cp_tx_bw tx_bw;

  // Fill NR SCS.
  tx_bw.nr_scs = to_subcarrier_spacing(asn1_tx_bw.nr_scs.to_string());

  // Fill NR NRB.
  tx_bw.nr_nrb = asn1_tx_bw.nr_nrb.to_number();

  return tx_bw;
}

/// \brief Convert XNAP ASN.1 to \c cu_cp_nr_freq_info.
/// \param[in] asn1_nr_freq_info The ASN.1 type NR freq info.
/// \return The common type NR freq info.
inline cu_cp_nr_freq_info asn1_to_nr_freq_info(const asn1::xnap::nr_freq_info_s& asn1_nr_freq_info)
{
  cu_cp_nr_freq_info nr_freq_info;

  // Fill NR ARFCN.
  nr_freq_info.nr_arfcn = asn1_nr_freq_info.nr_arfcn;

  // Fill SUL info.
  if (asn1_nr_freq_info.sul_info_present) {
    cu_cp_sul_info sul_info;

    // Fill SUL NR ARFCN.
    sul_info.sul_nr_arfcn = asn1_nr_freq_info.sul_info.sul_freq_info;

    // Fill SUL TX BW.
    sul_info.sul_tx_bw = asn1_to_tx_bw(asn1_nr_freq_info.sul_info.sul_tx_bw);

    nr_freq_info.sul_info = sul_info;
  }

  // Fill freq band list NR.
  for (const auto& asn1_freq_band : asn1_nr_freq_info.freq_band_list) {
    cu_cp_freq_band_nr_item freq_band;

    // Fill freq band ind.
    freq_band.freq_band_ind_nr = asn1_freq_band.nr_freq_band;

    // Fill supported SUL band list.
    for (const auto& asn1_sul_band : asn1_freq_band.supported_sul_band_list) {
      freq_band.supported_sul_band_list.push_back(cu_cp_supported_sul_freq_band_item{asn1_sul_band.sul_band_item});
    }

    nr_freq_info.freq_band_list_nr.push_back(freq_band);
  }

  return nr_freq_info;
}

/// \brief Convert F1AP ASN.1 to \c cu_cp_nr_mode_info.
/// \param[in] asn1_nr_mode_info The ASN.1 type NR mode info.
/// \return The common type NR mode info.
inline cu_cp_nr_mode_info asn1_to_nr_mode_info(const asn1::xnap::nr_mode_info_c& asn1_nr_mode_info)
{
  cu_cp_nr_mode_info nr_mode_info;

  // FDD.
  if (asn1_nr_mode_info.type() == asn1::xnap::nr_mode_info_c::types_opts::fdd) {
    const asn1::xnap::nr_mode_info_fdd_s& asn1_fdd_info = asn1_nr_mode_info.fdd();

    cu_cp_fdd_info fdd_info;

    // Fill UL NR freq info.
    fdd_info.ul_nr_freq_info = asn1_to_nr_freq_info(asn1_fdd_info.ul_nr_freq_info);

    // Fill DL NR freq info.
    fdd_info.dl_nr_freq_info = asn1_to_nr_freq_info(asn1_fdd_info.dl_nr_freq_info);

    // Fill UL TX BW.
    fdd_info.ul_tx_bw = asn1_to_tx_bw(asn1_fdd_info.ul_nr_transmisson_bw);

    // Fill DL TX BW.
    fdd_info.dl_tx_bw = asn1_to_tx_bw(asn1_fdd_info.dl_nr_transmisson_bw);

    nr_mode_info = fdd_info;
  } else if (asn1_nr_mode_info.type() == asn1::xnap::nr_mode_info_c::types_opts::tdd) {
    const asn1::xnap::nr_mode_info_tdd_s& asn1_tdd_info = asn1_nr_mode_info.tdd();

    cu_cp_tdd_info tdd_info;

    // Fill NR freq info.
    tdd_info.nr_freq_info = asn1_to_nr_freq_info(asn1_tdd_info.nr_freq_info);

    // Fill TX BW.
    tdd_info.tx_bw = asn1_to_tx_bw(asn1_tdd_info.nr_transmisson_bw);

    nr_mode_info = tdd_info;
  } else {
    report_fatal_error("Invalid NR mode.");
  }

  return nr_mode_info;
}

} // namespace ocudu::ocucp
