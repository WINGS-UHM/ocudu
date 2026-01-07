/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "lib/nrppa/ue_context/nrppa_ue_context.h"
#include "test_helpers.h"
#include "ocudu/support/executors/manual_task_worker.h"
#include "ocudu/support/test_utils.h"
#include <cstdint>
#include <gtest/gtest.h>

using namespace ocudu;
using namespace ocucp;

// Test class.
class nrppa_ue_context_list_test : public nrppa_ue_context_list
{
public:
  nrppa_ue_context_list_test(ocudulog::basic_logger& logger_) : nrppa_ue_context_list(logger_) {}
};

/// Fixture class for NRPPA UE context.
class nrppa_ue_context_test : public ::testing::Test
{
protected:
  nrppa_ue_context_test()
  {
    nrppa_logger.set_level(ocudulog::basic_levels::debug);
    ocudulog::init();
  }

  ~nrppa_ue_context_test()
  {
    // Flush logger after each test.
    ocudulog::flush();
  }

  static ue_index_t generate_random_ue_index()
  {
    return uint_to_ue_index(
        test_rgen::uniform_int<uint64_t>(ue_index_to_uint(ue_index_t::min), ue_index_to_uint(ue_index_t::max) - 1));
  }

  static ran_ue_meas_id_t generate_random_ran_ue_meas_id()
  {
    return uint_to_ran_ue_meas_id(test_rgen::uniform_int<uint64_t>(ran_ue_meas_id_to_uint(ran_ue_meas_id_t::min),
                                                                   ran_ue_meas_id_to_uint(ran_ue_meas_id_t::max) - 1));
  }

  static lmf_ue_meas_id_t generate_random_lmf_ue_meas_id()
  {
    return uint_to_lmf_ue_meas_id(test_rgen::uniform_int<uint64_t>(lmf_ue_meas_id_to_uint(lmf_ue_meas_id_t::min),
                                                                   lmf_ue_meas_id_to_uint(lmf_ue_meas_id_t::max) - 1));
  }

  ocudulog::basic_logger&       nrppa_logger = ocudulog::fetch_basic_logger("nrppa");
  timer_manager                 timer_mng;
  manual_task_worker            ctrl_worker{128};
  timer_factory                 timers{timer_mng, ctrl_worker};
  dummy_nrppa_cu_cp_ue_notifier ue_notifier;
  nrppa_ue_context_list_test    ue_ctxt_list{nrppa_logger};
};

TEST_F(nrppa_ue_context_test, when_ue_added_then_ue_exists)
{
  ue_index_t       ue_index       = generate_random_ue_index();
  ran_ue_meas_id_t ran_ue_meas_id = generate_random_ran_ue_meas_id();
  lmf_ue_meas_id_t lmf_ue_meas_id = generate_random_lmf_ue_meas_id();

  ue_ctxt_list.add_ue(ue_index, ran_ue_meas_id, lmf_ue_meas_id, ue_notifier, timer_mng, ctrl_worker);

  ASSERT_TRUE(ue_ctxt_list.contains(ue_index));

  ASSERT_EQ(ue_ctxt_list[ue_index].ue_ids.ran_ue_meas_id, ran_ue_meas_id);
  ASSERT_EQ(ue_ctxt_list[ue_index].ue_ids.ue_index, ue_index);
}

TEST_F(nrppa_ue_context_test, when_ue_not_added_then_ue_doesnt_exist)
{
  ue_index_t ue_index = generate_random_ue_index();

  ASSERT_FALSE(ue_ctxt_list.contains(ue_index));
}

TEST_F(nrppa_ue_context_test, when_ue_exists_then_removal_succeeds)
{
  ue_index_t       ue_index       = generate_random_ue_index();
  ran_ue_meas_id_t ran_ue_meas_id = generate_random_ran_ue_meas_id();
  lmf_ue_meas_id_t lmf_ue_meas_id = generate_random_lmf_ue_meas_id();

  ue_ctxt_list.add_ue(ue_index, ran_ue_meas_id, lmf_ue_meas_id, ue_notifier, timer_mng, ctrl_worker);

  // Test removal.
  ue_ctxt_list.remove_ue_context(ue_index);

  ASSERT_FALSE(ue_ctxt_list.contains(ue_index));
}

TEST_F(nrppa_ue_context_test, when_ue_is_added_then_next_ue_id_is_increased)
{
  ue_index_t       ue_index       = generate_random_ue_index();
  ran_ue_meas_id_t ran_ue_meas_id = ue_ctxt_list.allocate_ran_ue_meas_id().value();
  lmf_ue_meas_id_t lmf_ue_meas_id = generate_random_lmf_ue_meas_id();

  ASSERT_EQ((unsigned)ran_ue_meas_id, (unsigned)ran_ue_meas_id_t::min);

  ue_ctxt_list.add_ue(ue_index, ran_ue_meas_id, lmf_ue_meas_id, ue_notifier, timer_mng, ctrl_worker);

  // Remove UE.
  ue_ctxt_list.remove_ue_context(ue_index);

  ASSERT_FALSE(ue_ctxt_list.contains(ue_index));

  ASSERT_EQ((unsigned)ue_ctxt_list.allocate_ran_ue_meas_id().value(), (unsigned)ran_ue_meas_id_t::min + 1);
}
