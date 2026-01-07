#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

add_subdirectory(consumers)

set(SOURCES
        cu_cp_metrics_producer.cpp
        cu_cp_metrics_consumers.cpp)

add_library(ocudu_cu_cp_unit_metrics_helpers STATIC ${SOURCES})
target_include_directories(ocudu_cu_cp_unit_metrics_helpers PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(ocudu_cu_cp_unit_metrics_helpers PRIVATE ocudu_cu_cp_unit_metrics_consumers ocudu_cu_cp_metrics_json_generators_helpers)
