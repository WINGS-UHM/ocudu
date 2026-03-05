// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#pragma once

#include "ocudu/asn1/xnap/xnap_ies.h"
#include "ocudu/ran/cause/xnap_cause.h"
#include "ocudu/ran/cu_types.h"
#include "ocudu/ran/guami.h"
#include "ocudu/ran/nr_cgi.h"
#include "ocudu/ran/qos/qos_parameters.h"
#include "ocudu/ran/s_nssai.h"
#include "ocudu/security/security.h"

namespace ocudu::ocucp {

/// \brief Convert \c xnap_cause_t type to XNAP cause.
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
/// \brief Converts internal CGI struct into ASN.1 encoded NR-CGI.
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

/// \brief Converts common QoS flow level QoS parameters to ASN.1.
/// \param[in] qos_flow_level_params Common type QoS flow level QoS parameters.
/// \return ASN.1 QoS flow level QoS parameters.
inline asn1::xnap::qos_flow_level_qos_params_s
fill_asn1_qos_flow_info_item(const qos_flow_level_qos_parameters& qos_flow_level_params)
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
    asn1_qos_flow_level_params.gbr_qos_flow_info.max_flow_bit_rate_dl =
        qos_flow_level_params.gbr_qos_info.value().max_br_dl;
    asn1_qos_flow_level_params.gbr_qos_flow_info.max_flow_bit_rate_ul =
        qos_flow_level_params.gbr_qos_info.value().max_br_ul;
    asn1_qos_flow_level_params.gbr_qos_flow_info.guaranteed_flow_bit_rate_dl =
        qos_flow_level_params.gbr_qos_info.value().gbr_dl;
    asn1_qos_flow_level_params.gbr_qos_flow_info.guaranteed_flow_bit_rate_ul =
        qos_flow_level_params.gbr_qos_info.value().gbr_ul;
    if (qos_flow_level_params.gbr_qos_info.value().max_packet_loss_rate_dl.has_value()) {
      asn1_qos_flow_level_params.gbr_qos_flow_info.max_packet_loss_rate_dl_present = true;
      asn1_qos_flow_level_params.gbr_qos_flow_info.max_packet_loss_rate_dl =
          qos_flow_level_params.gbr_qos_info.value().max_packet_loss_rate_dl.value();
    }
    if (qos_flow_level_params.gbr_qos_info.value().max_packet_loss_rate_ul.has_value()) {
      asn1_qos_flow_level_params.gbr_qos_flow_info.max_packet_loss_rate_ul_present = true;
      asn1_qos_flow_level_params.gbr_qos_flow_info.max_packet_loss_rate_ul =
          qos_flow_level_params.gbr_qos_info.value().max_packet_loss_rate_ul.value();
    }
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

} // namespace ocudu::ocucp
