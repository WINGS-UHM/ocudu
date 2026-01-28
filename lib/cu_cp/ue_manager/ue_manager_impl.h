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

#include "../adapters/ngap_adapters.h"
#include "../adapters/rrc_ue_adapters.h"
#include "../cell_meas_manager/measurement_context.h"
#include "cu_cp_ue_impl.h"
#include "ue_metrics_handler.h"
#include "ue_task_scheduler_impl.h"
#include "ocudu/cu_cp/cu_cp_configuration.h"
#include "ocudu/cu_cp/security_manager_config.h"
#include "ocudu/cu_cp/ue_configuration.h"
#include "ocudu/ran/i_rnti.h"
#include "ocudu/ran/plmn_identity.h"
#include <optional>
#include <set>
#include <unordered_map>

namespace ocudu::ocucp {

class ue_manager : public ue_metrics_handler
{
public:
  explicit ue_manager(const cu_cp_configuration& cu_cp_cfg);

  /// Stop UE activity.
  void stop();

  /// \brief Remove the UE context with the given UE index.
  /// \param[in] ue_index Index of the UE to be removed.
  void remove_ue(ue_index_t ue_index);

  bool set_plmn(ue_index_t ue_index, const plmn_identity& plmn);

  /// \brief Add PLMNs to block and reject new UE connections for these.
  /// \param[in] plmns PLMN identities of UEs to be rejected.
  void add_blocked_plmns(const std::vector<plmn_identity>& plmns);

  /// \brief Remove blocked PLMNs and re-allow new UE connections for these.
  /// \param[in] plmns PLMN identities of the UEs to be allowed again.
  void remove_blocked_plmns(const std::vector<plmn_identity>& plmns);

  /// \brief Find the UEs with the given PLMN identity.
  /// \param[in] plmn PLMN identity of the UEs to be found.
  /// \return Vector of pointers to the found UEs.
  std::vector<cu_cp_ue*> find_ues(plmn_identity plmn);

  /// \brief Get the UE index of the UE.
  /// \param[in] pci The PCI of the cell the UE is/was connected to.
  /// \param[in] c_rnti The RNTI of the UE.
  ue_index_t get_ue_index(pci_t pci, rnti_t c_rnti);

  /// \brief Get the UE index of the UE.
  /// \param[in] full_i_rnti The Full-I-RNTI of the UE.
  ue_index_t get_ue_index(full_i_rnti_t full_i_rnti);

  /// \brief Get the UE index of the UE.
  /// \param[in] short_i_rnti The Short-I-RNTI of the UE.
  ue_index_t get_ue_index(short_i_rnti_t short_i_rnti);

  /// \brief Set the UE with the given UE index to inactive state.
  /// \param[in] ue_index Index of the UE to be set to inactive.
  /// \return The Full- and Short-I-RNTI assigned to the UE for resumption, or std::nullopt if not applicable.
  std::optional<i_rntis_t> set_inactive(ue_index_t ue_index);

  /// \brief Set the UE with the given UE index to active state.
  /// \param[in] ue_index Index of the UE to be set to active.
  void set_active(ue_index_t ue_index);

  /// \brief Get the Full-I-RNTI assigned to the UE with the given UE index.
  /// \param[in] ue_index Index of the UE.
  /// \return The Full-I-RNTI assigned to the UE, or std::nullopt if not applicable.
  std::optional<full_i_rnti_t> get_full_i_rnti(ue_index_t ue_index);

  /// \brief Get the CU-CP UE configuration stored in the UE manager.
  /// \return The CU-CP UE configuration.
  const ue_configuration& get_ue_config() const { return ue_config; }

  /// \brief Get the number of UEs.
  /// \return Number of UEs.
  size_t get_nof_ues() const { return ues.size(); }

  // common

  /// \brief Find the UE with the given UE index.
  /// \param[in] ue_index Index of the UE to be found.
  /// \return Pointer to the UE if found, nullptr otherwise.
  cu_cp_ue* find_ue(ue_index_t ue_index);

  /// \brief Get the UE task scheduler of the specified UE, even if the DU UE context is not created.
  /// \param[in] ue_index Index of the UE.
  /// \return Pointer to the UE task scheduler if found, nullptr otherwise.
  ue_task_scheduler* find_ue_task_scheduler(ue_index_t ue_index);

  // du processor

  /// \brief Allocate resources for the UE in the CU-CP.
  /// \param[in] du_index Index of the DU the UE is connected to.
  /// \param[in] du_id The gNB-DU ID of the DU the UE is connected to.
  /// \param[in] pci The PCI of the cell the UE is connected to.
  /// \param[in] rnti The RNTI of the UE.
  /// \param[in] pcell_index The index of the PCell the UE is connected to.
  /// \return ue_index of the created UE or ue_index_t::invalid in case of failure.
  ue_index_t add_ue(du_index_t                     du_index,
                    std::optional<gnb_du_id_t>     du_id       = std::nullopt,
                    std::optional<pci_t>           pci         = std::nullopt,
                    std::optional<rnti_t>          rnti        = std::nullopt,
                    std::optional<du_cell_index_t> pcell_index = std::nullopt);

  /// \brief Set the DU context of the UE.
  /// \param[in] ue_index Index of the UE.
  /// \param[in] du_id The gNB-DU ID of the DU the UE is connected to.
  /// \param[in] pci The PCI of the cell the UE is connected to.
  /// \param[in] rnti The RNTI of the UE.
  /// \param[in] pcell_index The index of the PCell the UE is connected to.
  /// \return Pointer to the DU UE if found, nullptr otherwise.
  cu_cp_ue*
  set_ue_du_context(ue_index_t ue_index, gnb_du_id_t du_id, pci_t pci, rnti_t rnti, du_cell_index_t pcell_index);

  /// \brief Find the UE with the given UE index, thats DU context is set up.
  /// \param[in] ue_index Index of the UE to be found.
  /// \return Pointer to the DU UE if found, nullptr otherwise.
  cu_cp_ue* find_du_ue(ue_index_t ue_index);

  /// \brief Get the number of UEs connected to a specific DU.
  /// \return Number of UEs.
  size_t get_nof_du_ues(du_index_t du_index);

  // ngap

  // cu-cp ue manager
  /// \brief Get the NGAP to RRC UE adapter of the UE.
  ngap_rrc_ue_adapter& get_ngap_rrc_ue_adapter(ue_index_t ue_index)
  {
    ocudu_assert(ue_index != ue_index_t::invalid, "Invalid ue_index={}", ue_index);
    ocudu_assert(ues.find(ue_index) != ues.end(), "UE with ue_index={} does not exist", ue_index);

    return ues.at(ue_index).get_ngap_rrc_ue_adapter();
  }

  rrc_ue_ngap_adapter& get_rrc_ue_ngap_adapter(ue_index_t ue_index)
  {
    ocudu_assert(ue_index != ue_index_t::invalid, "Invalid ue_index={}", ue_index);
    ocudu_assert(ues.find(ue_index) != ues.end(), "UE with ue_index={} does not exist", ue_index);

    return ues.at(ue_index).get_rrc_ue_ngap_adapter();
  }

  rrc_ue_cu_cp_adapter& get_rrc_ue_cu_cp_adapter(ue_index_t ue_index)
  {
    ocudu_assert(ue_index != ue_index_t::invalid, "Invalid ue_index={}", ue_index);
    ocudu_assert(ues.find(ue_index) != ues.end(), "UE with ue_index={} does not exist", ue_index);

    return ues.at(ue_index).get_rrc_ue_cu_cp_adapter();
  }

  std::vector<cu_cp_metrics_report::ue_info> handle_ue_metrics_report_request() const override;

  ue_task_scheduler_manager& get_task_sched() { return ue_task_scheds; }

  cell_meas_manager_ue_context& get_measurement_context(ue_index_t ue_index)
  {
    ocudu_assert(ue_index != ue_index_t::invalid, "Invalid ue_index={}", ue_index);
    ocudu_assert(ues.find(ue_index) != ues.end(), "UE with ue_index={} does not exist", ue_index);

    return ues.at(ue_index).get_meas_context();
  }

protected:
  ue_index_t next_ue_index = ue_index_t::min;
  i_rntis_t  next_i_rntis;

private:
  /// \brief Get the next available UE index.
  /// \return The UE index.
  ue_index_t allocate_ue_index();

  void increase_next_ue_index()
  {
    if (next_ue_index == ue_index_t::max) {
      // Reset cu ue f1ap id counter.
      next_ue_index = ue_index_t::min;
    } else {
      // Increase cu ue f1ap id counter.
      next_ue_index = uint_to_ue_index(ue_index_to_uint(next_ue_index) + 1);
    }
  }

  /// \brief Get the next available Full- and Short-I-RNTI.
  /// \return The Full- and Short-I-RNTI if available, std::nullopt otherwise.
  std::optional<i_rntis_t> allocate_i_rntis();

  void increase_full_i_rnti(full_i_rnti_t& full_i_rnti) const
  {
    if (full_i_rnti.value() == full_i_rnti.max()) {
      // Reset Full-I-RNTI counter.
      full_i_rnti = full_i_rnti_t{cu_cp_config.node.gnb_id.id, 0, ue_config.nof_i_rnti_ue_bits};
    } else {
      // Increase Full-I-RNTI counter.
      uint32_t next_ue_id = (full_i_rnti.value() - (cu_cp_config.node.gnb_id.id >> ue_config.nof_i_rnti_ue_bits)) + 1;
      full_i_rnti         = full_i_rnti_t{cu_cp_config.node.gnb_id.id, next_ue_id, ue_config.nof_i_rnti_ue_bits};
    }
  }

  void increase_short_i_rnti(short_i_rnti_t& short_i_rnti) const
  {
    if (short_i_rnti.value() == short_i_rnti.max()) {
      // Reset Short-I-RNTI counter.
      short_i_rnti = short_i_rnti_t{cu_cp_config.node.gnb_id.id, 0, ue_config.nof_i_rnti_ue_bits};
    } else {
      // Increase Short-I-RNTI counter.
      uint32_t next_ue_id = (short_i_rnti.value() - (cu_cp_config.node.gnb_id.id >> ue_config.nof_i_rnti_ue_bits)) + 1;
      short_i_rnti        = short_i_rnti_t{cu_cp_config.node.gnb_id.id, next_ue_id, ue_config.nof_i_rnti_ue_bits};
    }
  }

  ocudulog::basic_logger&       logger = ocudulog::fetch_basic_logger("CU-UEMNG");
  const cu_cp_configuration     cu_cp_config;
  const ue_configuration        ue_config;
  const up_resource_manager_cfg up_config;
  const security_manager_config sec_config;
  const unsigned                max_nof_ues;

  // Manager of UE task schedulers.
  ue_task_scheduler_manager ue_task_scheds;

  // Container of UE contexts handled by the CU-CP.
  std::unordered_map<ue_index_t, cu_cp_ue> ues;

  // UE index lookups.
  std::map<std::tuple<pci_t, rnti_t>, ue_index_t> pci_rnti_to_ue_index;     // ue_indexes indexed by pci and rnti
  std::map<short_i_rnti_t, ue_index_t>            short_i_rnti_to_ue_index; // ue_indexes indexed by short_i_rnti
  std::map<full_i_rnti_t, ue_index_t>             full_i_rnti_to_ue_index;  // ue_indexes indexed by full_i_rnti

  std::set<plmn_identity> blocked_plmns;
};

} // namespace ocudu::ocucp
