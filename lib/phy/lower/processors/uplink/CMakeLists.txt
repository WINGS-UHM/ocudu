#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

add_subdirectory(prach)
add_subdirectory(puxch)

add_library(ocudu_lower_phy_uplink_processor STATIC
        uplink_processor_factories.cpp
        uplink_processor_impl.cpp)
target_link_libraries(ocudu_lower_phy_uplink_processor
        ocudu_lower_phy_prach_processor
        ocudu_lower_phy_puxch_processor)