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

#include "ocudu/adt/bounded_integer.h"
#include "ocudu/ran/ntn.h"
#include "ocudu/ran/plmn_identity.h"
#include "ocudu/ran/sib/cell_reselection.h"
#include <variant>
#include <vector>

namespace ocudu {

struct cell_selection_info {
  /// \brief \c q-RxLevMin, part of \c cellSelectionInfo, \c SIB1, TS 38.311, in dBm.
  /// Indicates the required minimum received RSRP level for cell selection/re-selection (see \c Q-RxLevMin, TS 38.311).
  bounded_integer<int, -70, -22> q_rx_lev_min = -70;
  /// \brief \c q-QualMin, part of \c cellSelectionInfo, \c SIB1, TS 38.311, in dB.
  /// Indicates the required minimum received RSRQ level for cell selection/re-selection (see \c Q-QualMin, TS 38.311).
  bounded_integer<int, -43, -12> q_qual_min = -20;
};

struct cell_access_related_info {
  /// Additional PLMNs that the UE can use to access the cell besides the cell primary PLMN. See TS 38.331, \c SIB1.
  std::vector<plmn_identity> additional_plmns;
};

enum class sib_type : uint8_t {
  sib1        = 1,
  sib2        = 2,
  sib3        = 3,
  sib4        = 4,
  sib5        = 5,
  sib6        = 6,
  sib7        = 7,
  sib8        = 8,
  sib19       = 19,
  sib_invalid = 255
};

/// Cell reselection information common to all cell reselection types.
struct sib2_info {
  // Parameter "Qhyst" in TS 38.304. Hysteresis value for ranking criteria.
  q_hyst_t q_hyst;
  /// \brief Parameter "ThreshServing, LowP" in TS 38.304.
  ///
  /// Rx level threshold used by the UE on the serving cell when reselecting towards a lower priority RAT/frequency.
  reselection_threshold_t thresh_serving_low_p;
  // Integer part of the cell reselection priority for the frequency of this cell.
  cell_reselection_priority_t cell_reselection_priority;
  /// \brief Parameter "QrxLevMin" in TS 38.304.
  ///
  /// Minimum required Rx level in the cell.
  q_rx_lev_min_t q_rx_lev_min;
  /// \brief Parameter "SIntraSearchP" in TS 38.304.
  ///
  /// Rx level threshold for intra-frequency measurements.
  reselection_threshold_t s_intra_search_p;
  /// \brief Parameter "TreselectionNR" in TS 38.304.
  ///
  /// Cell reselection timer value "TreselectionRAT" for NR.
  t_reselection_t t_reselection_nr;
};

/// Intra-frequency cell reselection information SIB contents (see TS38.331 Section 6.3.2, Information Element \e
/// SIB3).
struct sib3_info {
  /// List of intra-frequency neighbouring cells with specific cell reselection parameters.
  std::vector<intra_freq_neigh_cell_info> intra_freq_neigh_cell_list;
  /// List of excluded intra-frequency neighbouring cells.
  std::vector<pci_range_t> intra_freq_excluded_cell_list;
};

/// Inter-frequency cell reselection information SIB contents (see TS38.331 Section 6.3.2, Information Element \e
/// SIB4).
struct sib4_info {
  /// List of neighbouring carrier frequencies and frequency specific cell reselection information.
  std::vector<inter_freq_carrier_freq_info> inter_freq_carrier_freq_list;
};

/// Intra-RAT cell reselection information SIB contents (see TS38.331 Section 6.3.2, Information Element \e SIB5).
struct sib5_info {
  /// \brief Parameter "TreselectionEUTRA" in TS 38.304.
  ///
  /// Cell reselection timer value "TreselectionRAT" for E-UTRAN.
  t_reselection_t t_reselection_eutra;
  /// List of carrier frequencies of E-UTRA.
  std::vector<carrier_freq_eutra> carrier_freq_list_eutra;
};

/// ETWS primary notification SIB contents (see TS38.331 Section 6.3.2, Information Element \e SIB6).
struct sib6_info {
  /// \brief Parameter "messageIdentifier".
  ///
  /// It carries the NGAP Message Identifier IE (see TS38.413, Section 9.3.1.35), which is set according to the Message
  /// Identifier field of the Cell Broadcast Service (CBS) ETWS Primary Notification Message (see TS23.041,
  /// Section 9.4.3.3.4). It identifies the source and type of the warning message.
  unsigned message_id;
  /// \brief Parameter "serialNumber".
  ///
  /// It carries the NGAP Serial Number IE (see TS38.413, Section 9.3.1.36), which is set according to the Serial
  /// Number field of the Cell Broadcast Service (CBS) ETWS Primary Notification Message (see TS23.041,
  /// Section 9.4.3.3.3). It identifies a specific warning message from other message of the same type. It must be
  /// incremented when a new warning message is broadcast.
  unsigned serial_number;
  /// \brief Parameter "warningType".
  ///
  /// It carries the NGAP Warning Type IE (see TS38.413, Section 9.3.1.39), which is set according to the Warning
  /// Type field of the Cell Broadcast Service (CBS) ETWS Primary Notification Message (see TS23.041,
  /// Section 9.4.3.3.5). It identifies the type of ETWS warning between Earthquake, Tsunami, Earthquake and Tsunami and
  /// Test. It also carries the Emergency User Alert and Popup fields, which activate the audible warning and
  /// notification popup features on the UEs.
  unsigned warning_type;
};

/// ETWS secondary notification SIB contents (see TS38.331 Section 6.3.2, Information Element \e SIB7).
struct sib7_info {
  /// \brief Parameter "messageIdentifier".
  ///
  /// It carries the Message Identifier NGAP IE (see TS38.413, Section 9.3.1.35), which is set according to the Message
  /// Identifier field of the Cell Broadcast Service (CBS) message (see TS23.041, Section 9.4.3.2.1). It identifies the
  /// source and type of the warning message.
  unsigned message_id;
  /// \brief Parameter "serialNumber".
  ///
  /// It carries the Serial Number NGAP IE (see TS38.413, Section 9.3.1.36), which is set according to the Serial
  /// Number field of the Cell Broadcast Service (CBS) message (see TS23.041, Section 9.4.3.2.2). It identifies a
  /// specific warning message from other message of the same type. It must be incremented when a new warning message is
  /// broadcast.
  unsigned serial_number;
  /// \brief Parameter "warningMessageSegment".
  ///
  /// It carries a segment of the ETWS warning message contents NGAP IE defined in TS TS38.413, Section 9.3.1.41, which
  /// has the contents of a CBS message CB Data IE (See TS23.041 Section 9.4.3.2.4).
  std::string warning_message_segment;
  /// \brief Parameter "dataCodingScheme".
  ///
  /// It carries the Data Coding Scheme NGAP IE (see TS38.413, Section 9.3.1.41), which is set according to the Data
  /// Coding Scheme field of the Cell Broadcast Service (CBS) message (see TS23.041, Section 9.4.3.2.3). It identifies
  /// the coding and the language of the warning message as per TS23.041 Section 9.4.2.2.4.
  unsigned data_coding_scheme;
};

/// CMAS notification SIB contents (see TS38.331 Section 6.3.2, Information Element \e SIB8).
struct sib8_info {
  /// \brief Parameter "messageIdentifier".
  ///
  /// It carries the Message Identifier NGAP IE (see TS38.413, Section 9.3.1.35), which is set according to the Message
  /// Identifier field of the Cell Broadcast Service (CBS) message (see TS23.041, Section 9.4.3.2.1). It identifies the
  /// source and type of the warning message.
  unsigned message_id;
  /// \brief Parameter "serialNumber".
  ///
  /// It carries the Serial Number NGAP IE (see TS38.413, Section 9.3.1.36), which is set according to the Serial
  /// Number field of the Cell Broadcast Service (CBS) message (see TS23.041, Section 9.4.3.2.2). It identifies a
  /// specific warning message from other message of the same type. It must be incremented when a new warning message is
  /// broadcast.
  unsigned serial_number;
  /// \brief Parameter "warningMessageSegment".
  ///
  /// It carries a segment of the CMAS warning message contents NGAP IE defined in TS TS38.413, Section 9.3.1.41, which
  /// has the contents of a CBS message CB Data IE (See TS23.041 Section 9.4.3.2.4).
  std::string warning_message_segment;
  /// \brief Parameter "dataCodingScheme".
  ///
  /// It carries the Data Coding Scheme NGAP IE (see TS38.413, Section 9.3.1.41), which is set according to the Data
  /// Coding Scheme field of the Cell Broadcast Service (CBS) message (see TS23.041, Section 9.4.3.2.3). It identifies
  /// the coding and the language of the warning message as per TS23.041 Section 9.4.2.2.4.
  unsigned data_coding_scheme;
};

struct sib19_info {
  // This user provided constructor is added here to fix a Clang compilation error related to the use of nested types
  // with std::optional.
  sib19_info() {}

  std::optional<unsigned>                                                distance_thres;
  std::optional<geodetic_coordinates_t>                                  ref_location;
  std::optional<uint64_t>                                                t_service;
  std::optional<uint16_t>                                                cell_specific_koffset;
  std::optional<std::variant<ecef_coordinates_t, orbital_coordinates_t>> ephemeris_info;
  std::optional<epoch_time_t>                                            epoch_time;
  std::optional<uint16_t>                                                k_mac;
  std::optional<ta_info_t>                                               ta_info;
  std::optional<uint16_t>                                                ntn_ul_sync_validity_dur;
  std::optional<ntn_polarization_t>                                      polarization;
  std::optional<bool>                                                    ta_report;
};

/// \brief Variant type that can hold different types of SIBs that go in a SI message.
using sib_info = std::variant<sib2_info, sib3_info, sib4_info, sib5_info, sib6_info, sib7_info, sib8_info, sib19_info>;

inline sib_type get_sib_info_type(const sib_info& sib)
{
  if (std::holds_alternative<sib2_info>(sib)) {
    return sib_type::sib2;
  }
  if (std::holds_alternative<sib3_info>(sib)) {
    return sib_type::sib3;
  }
  if (std::holds_alternative<sib4_info>(sib)) {
    return sib_type::sib4;
  }
  if (std::holds_alternative<sib5_info>(sib)) {
    return sib_type::sib5;
  }
  if (std::holds_alternative<sib6_info>(sib)) {
    return sib_type::sib6;
  }
  if (std::holds_alternative<sib7_info>(sib)) {
    return sib_type::sib7;
  }
  if (std::holds_alternative<sib8_info>(sib)) {
    return sib_type::sib8;
  }
  if (std::holds_alternative<sib19_info>(sib)) {
    return sib_type::sib19;
  }
  return sib_type::sib_invalid;
}

/// \brief This struct contains the information required for the scheduling of the SI messages by the network.
struct si_message_sched_info {
  /// List of SIBs (sib2, sib3, ...) included in this SI message. The list has at most 32 elements.
  std::vector<sib_type> sib_mapping_info;
  /// Periodicity of the SI-message in radio frames. Values: {8, 16, 32, 64, 128, 256, 512}.
  unsigned si_period_radio_frames = 32;
  /// SI window position of the associated SI-message. The network provides si-WindowPosition in an ascending order,
  /// i.e. si-WindowPosition in the subsequent entry in schedulingInfoList2 has always value higher than in the previous
  /// entry of schedulingInfoList2. See TS 38.331, \c SchedulingInfo2-r17. Values: {1,...,256}.
  /// \remark This field is only applicable for release 17 \c SI-SchedulingInfo.
  std::optional<unsigned> si_window_position;
};

/// This struct contains the information required for the generation of the SI messages sent by the network and the
/// generation of the SIB1 "SI-SchedulingInfo" field of the SIB1. See TS 38.331, "SystemInformation" and
/// "SI-SchedulingInfo".
struct si_scheduling_info_config {
  /// The length of the SI scheduling window, in slots. It is always shorter or equal to the period of the SI message.
  /// Values: {5, 10, 20, 40, 80, 160, 320, 640, 1280}.
  unsigned si_window_len_slots;
  /// List of SI-messages and associated scheduling information.
  std::vector<si_message_sched_info> si_sched_info;
  /// Information included in each SIB that is scheduled as part of one of the SI-messages.
  std::vector<sib_info> sibs;
};

/// This struct contains the information required for the generation of the SIB1 "UE-TimersAndConstants" field of the
/// SIB1. See TS 38.331 section 7.
struct ue_timers_and_constants_config {
  /// t300
  /// Values (in ms): {100, 200, 300, 400, 600, 1000, 1500, 2000}
  std::chrono::milliseconds t300;
  /// t301
  /// Values (in ms): {100, 200, 300, 400, 600, 1000, 1500, 2000}
  std::chrono::milliseconds t301;
  /// \brief Timer triggered by UE upon detection of N310 consecutive out-of-sync indications from lower layers, as
  /// per TS 38.331, 7.1.1. Values: {ms0, ms50, ms100, ms200, ms500, ms1000, ms2000}.
  std::chrono::milliseconds t310;
  /// n310
  /// Values: {1, 2, 3, 4, 6, 8, 10, 20}
  unsigned n310;
  /// \brief Timer triggered by UE upon initiating RRC connection reestablishment procedure, as per TS 38.331 7.1.1.
  /// Values: {ms1000, ms3000, ms5000, ms10000, ms15000, ms20000, ms30000}.
  std::chrono::milliseconds t311;
  /// n311
  /// Values: {1, 2, 3, 4, 5, 6, 8, 10}
  unsigned n311;
  /// t319
  /// Values (in ms): {100, 200, 300, 400, 600, 1000, 1500, 2000}
  std::chrono::milliseconds t319;
};

} // namespace ocudu
