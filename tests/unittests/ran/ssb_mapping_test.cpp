// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#include "ocudu/ran/ssb/ssb_mapping.h"
#include "ocudu/support/test_utils.h"

using namespace ocudu;

void get_ssb_crbs()
{
  // Test case with subcarrier_spacing::kHz15 with k_SSB == 0.
  crb_interval expected_crbs = crb_interval{0, 20};
  TESTASSERT(expected_crbs == get_ssb_crbs(subcarrier_spacing::kHz15 /*ssb_scs*/,
                                           subcarrier_spacing::kHz15 /*scs_common*/,
                                           ssb_offset_to_pointA{0},
                                           ssb_subcarrier_offset{0}));

  // Test case with subcarrier_spacing::kHz15 with k_SSB > 0.
  expected_crbs = crb_interval{0, 21};
  TESTASSERT(expected_crbs == get_ssb_crbs(subcarrier_spacing::kHz15 /*ssb_scs*/,
                                           subcarrier_spacing::kHz15 /*scs_common*/,
                                           ssb_offset_to_pointA{0},
                                           ssb_subcarrier_offset{1}));

  expected_crbs = crb_interval{25, 45};
  TESTASSERT(expected_crbs == get_ssb_crbs(subcarrier_spacing::kHz15 /*ssb_scs*/,
                                           subcarrier_spacing::kHz15 /*scs_common*/,
                                           ssb_offset_to_pointA{25},
                                           ssb_subcarrier_offset{0}));

  expected_crbs = crb_interval{25, 46};
  TESTASSERT(expected_crbs == get_ssb_crbs(subcarrier_spacing::kHz15 /*ssb_scs*/,
                                           subcarrier_spacing::kHz15 /*scs_common*/,
                                           ssb_offset_to_pointA{25},
                                           ssb_subcarrier_offset{5}));

  // Test case with subcarrier_spacing::kHz30 with k_SSB == 0.
  expected_crbs = crb_interval{0, 20};
  TESTASSERT(expected_crbs == get_ssb_crbs(subcarrier_spacing::kHz30 /*ssb_scs*/,
                                           subcarrier_spacing::kHz30 /*scs_common*/,
                                           ssb_offset_to_pointA{0},
                                           ssb_subcarrier_offset{0}));

  // Test case with subcarrier_spacing::kHz30 with k_SSB > 0.
  expected_crbs = crb_interval{0, 21};
  TESTASSERT(expected_crbs == get_ssb_crbs(subcarrier_spacing::kHz30 /*ssb_scs*/,
                                           subcarrier_spacing::kHz30 /*scs_common*/,
                                           ssb_offset_to_pointA{0},
                                           ssb_subcarrier_offset{2}));

  expected_crbs = crb_interval{6, 26};
  TESTASSERT(expected_crbs == get_ssb_crbs(subcarrier_spacing::kHz30 /*ssb_scs*/,
                                           subcarrier_spacing::kHz30 /*scs_common*/,
                                           ssb_offset_to_pointA{12},
                                           ssb_subcarrier_offset{0}));

  expected_crbs = crb_interval{6, 27};
  TESTASSERT(expected_crbs == get_ssb_crbs(subcarrier_spacing::kHz30 /*ssb_scs*/,
                                           subcarrier_spacing::kHz30 /*scs_common*/,
                                           ssb_offset_to_pointA{12},
                                           ssb_subcarrier_offset{2}));
}

int main()
{
  ocudulog::init();

  get_ssb_crbs();

  return 0;
}
