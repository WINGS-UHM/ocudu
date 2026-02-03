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

#include "segmented_sib_list.h"
#include "ocudu/adt/byte_buffer.h"
#include "ocudu/adt/lockfree_triple_buffer.h"
#include "ocudu/mac/cell_configuration.h"
#include "ocudu/mac/mac_cell_manager.h"
#include "ocudu/ocudulog/logger.h"
#include "ocudu/ran/slot_point_extended.h"
#include "ocudu/scheduler/result/pdsch_info.h"

namespace ocudu {

/// Entity responsible for fetching encoded SIB1 and SI messages based on scheduled SI grants.
class sib_pdu_assembler
{
public:
  /// Encoder of BCCH-DL-SCH messages that require dynamic fields to be updated at each transmission (e.g., HyperSFN).
  class bcch_dl_sch_msg_encoder
  {
  public:
    virtual ~bcch_dl_sch_msg_encoder() = default;

    /// \brief Get an encoded BCCH-DL-SCH message buffer for a given slot point and TBS.
    /// \param[in] sl_tx Transmission slot point.
    /// \param[in] tbs   Transport block size in bytes that was scheduled.
    /// \return The encoded BCCH-DL-SCH message buffer on success, otherwise an error containing the minimum TBS that
    /// should have been scheduled.
    virtual expected<span<const uint8_t>, units::bytes> encode(slot_point_extended sl_tx, units::bytes tbs) = 0;
  };

  class message_handler
  {
  public:
    virtual ~message_handler() = default;

    virtual si_version_type update(si_version_type si_version, const byte_buffer& pdu) = 0;

    /// \brief Enqueue encodes SI messages at proper Tx slots.
    virtual bool enqueue_si_pdu_updates(const mac_cell_sys_info_pdu_update& pdu_update_req) = 0;

    /// Retrieve encoded SI bytes for a given SI scheduling opportunity.
    virtual span<const uint8_t> get_pdu(slot_point_extended sl_tx, const sib_information& si_info) = 0;
  };

  sib_pdu_assembler(const mac_cell_sys_info_config& req);
  ~sib_pdu_assembler();

  /// Update the SIB1 and SI messages.
  void handle_si_change_request(const mac_cell_sys_info_config& req);

  /// \brief Retrieve the encoded SI message.
  span<const uint8_t> encode_si_pdu(slot_point_extended sl_tx, const sib_information& si_info);

  /// \brief Enqueue encodes SI messages at proper Tx slots.
  bool enqueue_si_message_pdu_updates(const mac_cell_sys_info_pdu_update& pdu_update_req);

private:
  using bcch_dl_sch_buffer = std::shared_ptr<const std::vector<uint8_t>>;

  /// Variant that can either hold a single BCCH payload, or multiple versions of such payload for segmented messages.
  using bcch_payload_type = std::variant<bcch_dl_sch_buffer, segmented_sib_list<bcch_dl_sch_buffer>>;

  /// A snapshot of a SIB1 and SI messages within a given SI change window.
  struct si_buffer_snapshot {
    unsigned version;
    /// Encoder used to generate BCCH-DL-SCH SIB1 messages.
    std::shared_ptr<bcch_dl_sch_msg_encoder>                sib1;
    std::vector<std::pair<units::bytes, bcch_payload_type>> si_msg_buffers;
  };

  class sib1_assembler;

  void save_buffers(si_version_type si_version, const mac_cell_sys_info_config& req);

  ocudulog::basic_logger& logger;

  // Last SI messages received by the assembler.
  static_vector<bcch_dl_sch_payload_type, MAX_SI_MESSAGES> last_si_messages;

  // SI buffers of last SI message update request.
  // Note: This member is only accessed from the control executor.
  si_buffer_snapshot last_cfg_buffers;

  // Buffers being transferred from configuration plane to assembler RT path.
  lockfree_triple_buffer<si_buffer_snapshot> pending;

  // SI buffers that are being currently encoded and sent to lower layers.
  // Note: This member is only accessed from the RT path.
  si_buffer_snapshot current_buffers;

  // Handler of SIB1s with extended functionality for eDRX.
  std::unique_ptr<sib1_assembler> sib1_hdlr;

  std::unique_ptr<message_handler> message_ext_handler;
};

/// \brief Instantiates an SI message extension handler.
/// \param[in] req    Request containing System Information signalled by the cell.
/// \return A pointer to the SI message extension handler on success, otherwise \c nullptr.
std::unique_ptr<sib_pdu_assembler::message_handler>
create_si_message_extension_handler(const mac_cell_sys_info_config& req);

} // namespace ocudu
