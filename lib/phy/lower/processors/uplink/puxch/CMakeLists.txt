#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

add_library(ocudu_lower_phy_puxch_processor STATIC
        puxch_processor_factories.cpp
        puxch_processor_impl.cpp)
target_link_libraries(ocudu_lower_phy_puxch_processor ocudu_lower_phy_modulation)