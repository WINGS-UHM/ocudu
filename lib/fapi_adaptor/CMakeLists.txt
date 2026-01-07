#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

add_subdirectory(mac)
add_subdirectory(phy)

set(PRECODING_SOURCES
        precoding_matrix_mapper.cpp
        precoding_matrix_repository.cpp
        precoding_matrix_table_generator.cpp
        )

set(UCI_SOURCES
        uci_part2_correspondence_generator.cpp
        uci_part2_correspondence_mapper.cpp
        uci_part2_correspondence_repository.cpp
        )

add_library(ocudu_fapi_precoding_matrix_tools STATIC ${PRECODING_SOURCES})
target_link_libraries(ocudu_fapi_precoding_matrix_tools ocudu_ran ocudulog)

add_library(ocudu_fapi_uci_part2_tools STATIC ${UCI_SOURCES})
target_link_libraries(ocudu_fapi_uci_part2_tools ocudu_ran ocudulog)
