#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

set(SOURCES
        mac_fapi_p5_fastpath_cell_operation_controller_impl.cpp
        mac_fapi_p5_sector_fastpath_adaptor_impl.cpp
        mac_fapi_p5_start_cell_procedure.cpp
        mac_fapi_p5_stop_cell_procedure.cpp
        p5_responses_handler.cpp)

add_library(ocudu_mac_to_fapi_p5_sector_fastpath_adaptor STATIC ${SOURCES})
target_link_libraries(ocudu_mac_to_fapi_p5_sector_fastpath_adaptor PRIVATE ocudu_support)
