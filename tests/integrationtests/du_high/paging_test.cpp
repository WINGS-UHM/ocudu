/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

/// \file
/// \brief Tests that check the transmission of Paging messages by the DU-high class.

#include "lib/f1ap/f1ap_asn1_packer.h"
#include "tests/integrationtests/du_high/test_utils/du_high_env_simulator.h"
#include "tests/test_doubles/f1ap/f1ap_test_message_validators.h"
#include "tests/unittests/gateways/test_helpers.h"
#include "ocudu/asn1/f1ap/common.h"
#include "ocudu/asn1/f1ap/f1ap_pdu_contents.h"
#include "ocudu/asn1/rrc_nr/sys_info.h"
#include "ocudu/f1ap/f1ap_message.h"
#include "ocudu/support/test_utils.h"

using namespace ocudu;
using namespace odu;
using namespace asn1::f1ap;

static bool is_edrx_enabled(const asn1::f1ap::gnb_du_served_cells_item_s& cell)
{
  if (not cell.gnb_du_sys_info_present) {
    return false;
  }
  asn1::rrc_nr::sib1_s sib1;
  {
    asn1::cbit_ref bref{cell.gnb_du_sys_info.sib1_msg};
    ocudu_assert(sib1.unpack(bref) == asn1::OCUDUASN_SUCCESS, "Failed to decode SIB1");
  }
  if (not sib1.non_crit_ext_present or not sib1.non_crit_ext.non_crit_ext_present or
      not sib1.non_crit_ext.non_crit_ext.non_crit_ext_present) {
    return false;
  }
  if (not sib1.non_crit_ext.non_crit_ext.non_crit_ext.hyper_sfn_r17_present or
      not sib1.non_crit_ext.non_crit_ext.non_crit_ext.edrx_allowed_idle_r17_present) {
    return false;
  }
  return true;
}

class paging_tester : public du_high_env_simulator, public testing::Test
{};

f1ap_message generate_paging_message(uint64_t five_g_tmsi, const nr_cell_global_id_t& nr_cgi)
{
  f1ap_message msg;
  msg.pdu.set_init_msg().load_info_obj(ASN1_F1AP_ID_PAGING);
  paging_s& paging = msg.pdu.init_msg().value.paging();

  // Add ue id idx value.
  // UE Identity Index value is defined as: UE_ID 5G-S-TMSI mod 1024  (see TS 38.304 section 7.1).
  paging->ue_id_idx_value.set_idx_len10().from_number(five_g_tmsi % 1024);

  // Add paging id.
  paging->paging_id.set_cn_ue_paging_id().set_five_g_s_tmsi().from_number(five_g_tmsi);

  // Add paging DRX.
  paging->paging_drx_present = true;
  paging->paging_drx         = asn1::f1ap::paging_drx_opts::v32;

  // Add paging cell list.
  asn1::protocol_ie_single_container_s<asn1::f1ap::paging_cell_item_ies_o> asn1_paging_cell_item_container;
  auto& asn1_paging_cell_item = asn1_paging_cell_item_container->paging_cell_item();
  asn1_paging_cell_item.nr_cgi.nr_cell_id.from_number(nr_cgi.nci.value());
  asn1_paging_cell_item.nr_cgi.plmn_id = nr_cgi.plmn_id.to_bytes();
  paging->paging_cell_list.push_back(asn1_paging_cell_item_container);

  return msg;
}

TEST_F(paging_tester, when_paging_message_is_received_its_relayed_to_ue)
{
  static constexpr uint64_t five_g_tmsi = 0x01011066fef7;

  // Check F1 Setup.
  ASSERT_TRUE(test_helpers::is_f1_setup_request_valid(cu_notifier.f1ap_ul_msgs.rbegin()->second));
  auto& f1_setup_req = cu_notifier.f1ap_ul_msgs.rbegin()->second.pdu.init_msg().value.f1_setup_request();
  ASSERT_TRUE(f1_setup_req->gnb_du_served_cells_list_present);
  auto& cell = f1_setup_req->gnb_du_served_cells_list[0].value().gnb_du_served_cells_item();
  ASSERT_FALSE(is_edrx_enabled(cell));

  // Receive F1AP paging message.
  cu_notifier.f1ap_ul_msgs.clear();
  const auto& du_cell_cfg = this->du_high_cfg.ran.cells[0];
  this->du_hi->get_f1ap_du().handle_message(generate_paging_message(five_g_tmsi, du_cell_cfg.nr_cgi));
  // Flag indicating whether UE is Paged or not.
  bool ue_is_paged{false};

  const unsigned MAX_COUNT = 50 * this->next_slot.nof_slots_per_frame();
  for (unsigned i = 0; i != MAX_COUNT; ++i) {
    this->run_slot();

    for (const auto& pg_grant : this->phy.cells[0].last_dl_res->dl_res->paging_grants) {
      const auto& pg_ue_it =
          std::find_if(pg_grant.paging_ue_list.begin(), pg_grant.paging_ue_list.end(), [](const paging_ue_info& ue) {
            return ue.paging_type_indicator == ocudu::paging_ue_info::cn_ue_paging_identity and
                   ue.paging_identity == five_g_tmsi;
          });
      if (pg_ue_it != pg_grant.paging_ue_list.end()) {
        ue_is_paged = true;
        break;
      }
    }
  }
  ASSERT_TRUE(ue_is_paged);
}

class edrx_paging_test : public du_high_env_simulator, public testing::Test
{
protected:
  edrx_paging_test() :
    du_high_env_simulator([]() {
      auto cfg                      = create_du_high_configuration(du_high_env_sim_params{});
      cfg.ran.cells[0].edrx_enabled = true;
      return cfg;
    }())
  {
  }
};

TEST_F(edrx_paging_test, when_edrx_enabled_then_sib1_contains_hyper_sfn)
{
  ASSERT_TRUE(test_helpers::is_f1_setup_request_valid(cu_notifier.f1ap_ul_msgs.rbegin()->second));
  auto& f1_setup_req = cu_notifier.f1ap_ul_msgs.rbegin()->second.pdu.init_msg().value.f1_setup_request();
  ASSERT_TRUE(f1_setup_req->gnb_du_served_cells_list_present);
  auto& cell = f1_setup_req->gnb_du_served_cells_list[0].value().gnb_du_served_cells_item();
  ASSERT_TRUE(is_edrx_enabled(cell));
}
