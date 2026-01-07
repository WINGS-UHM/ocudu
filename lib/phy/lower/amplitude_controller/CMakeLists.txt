#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

add_library(ocudu_lower_phy_amplitude_controller STATIC
        amplitude_controller_clipping_impl.cpp
        amplitude_controller_scaling_impl.cpp
        amplitude_controller_factories.cpp)
target_link_libraries(ocudu_lower_phy_amplitude_controller ocuduvec)
