#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

add_subdirectory(modulation)
add_subdirectory(processors)
add_subdirectory(amplitude_controller)

add_library(ocudu_lower_phy STATIC
        lower_phy_baseband_processor.cpp
        lower_phy_factory.cpp
        lower_phy_impl.cpp)
target_link_libraries(ocudu_lower_phy
        ocudu_dft
        ocudu_lower_phy_adaptors
        ocudu_lower_phy_amplitude_controller
        ocudu_lower_phy_downlink_processor
        ocudu_lower_phy_pdxch_processor
        ocudu_lower_phy_modulation
        ocudu_lower_phy_prach_processor
        ocudu_lower_phy_puxch_processor
        ocudu_lower_phy_uplink_processor)
