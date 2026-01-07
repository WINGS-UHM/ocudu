#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

add_subdirectory(consumers)

set(SOURCES
        flexible_o_du_metrics_builder.cpp
        flexible_o_du_metrics_consumers.cpp
        flexible_o_du_metrics_producer.cpp)

add_library(ocudu_flexible_o_du_unit_metrics_helpers STATIC ${SOURCES})
target_include_directories(ocudu_flexible_o_du_unit_metrics_helpers PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(ocudu_flexible_o_du_unit_metrics_helpers PRIVATE ocudu_metrics_json_generators ocudu_flexible_o_du_split_helpers_metrics_consumers)
