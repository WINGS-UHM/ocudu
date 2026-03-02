// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#include "lib/cu_cp/xnap_repository.h"
#include "tests/unittests/cu_cp/cu_cp_test_environment.h"
#include <gtest/gtest.h>

using namespace ocudu;
using namespace ocucp;

class cu_cp_xnap_repository_test : public cu_cp_test_environment, public ::testing::Test
{
public:
  cu_cp_xnap_repository_test() :
    cu_cp_test_environment(cu_cp_test_env_params{}), xnap_db(xnap_repository_config{get_cu_cp_cfg(), test_logger})
  {
  }

  xnap_repository xnap_db;

  gnb_id_t                default_gnb_id{.id = 411, .bit_length = 22};
  transport_layer_address default_peer_addr = transport_layer_address::create_from_string("127.0.0.1");
  guami_t default_guami{.plmn = plmn_identity::test_value(), .amf_set_id = 1, .amf_pointer = 1, .amf_region_id = 1};
  xnap_configuration xnap_cfg{.gnb_id           = default_gnb_id,
                              .tai_support_list = {default_supported_tracking_area},
                              .guami_list       = {default_guami}};
};

//----------------------------------------------------------------------------------//
// XNAP repository tests                                                            //
//----------------------------------------------------------------------------------//

TEST_F(cu_cp_xnap_repository_test,
       when_adding_xnap_then_xnap_is_added_to_repository_and_can_be_retrieved_by_index_and_peer_addr)
{
  // Add XNAP.
  xnap_interface* xnap = xnap_db.add_xnap(xnc_peer_index_t::min, default_peer_addr, xnap_cfg);
  ASSERT_TRUE(xnap != nullptr) << "Failed to add XNAP to repository";

  ASSERT_EQ(xnap_db.get_nof_xnaps(), 1U) << "Unexpected number of XNAPs in repository after adding XNAP";

  // Retrieve XNAP by index.
  xnap_interface* retrieved_xnap = xnap_db.find_xnap(xnc_peer_index_t::min);
  ASSERT_TRUE(retrieved_xnap != nullptr) << "Failed to retrieve XNAP by index";
  ASSERT_EQ(retrieved_xnap, xnap) << "Retrieved XNAP does not match added XNAP";

  // Retrieve XNAP by peer address.
  ASSERT_EQ(xnc_peer_index_t::min, xnap_db.find_xnap(default_peer_addr)) << "Failed to retrieve XNAP by peer address";
}
