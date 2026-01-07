#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

set(SOURCES
        p5_requests_handler.cpp
        phy_fapi_p5_sector_fastpath_adaptor_impl.cpp)

add_library(ocudu_phy_to_fapi_p5_sector_fastpath_adaptor STATIC ${SOURCES})
target_link_libraries(ocudu_phy_to_fapi_p5_sector_fastpath_adaptor PRIVATE ocudu_support)
