#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

set(SOURCES
        csi_rs.cpp
        pdcch.cpp
        pdsch.cpp
        prach.cpp
        prs.cpp
        pucch.cpp
        pusch.cpp
        srs.cpp
        ssb.cpp)

add_library(ocudu_fapi_phy_message_adaptors STATIC ${SOURCES})
target_link_libraries(ocudu_fapi_phy_message_adaptors ocuduvec ocudu_phy_support ocudu_fapi_precoding_matrix_tools ocudu_fapi_uci_part2_tools ocudu_upper_phy_support ocudu_ran)
