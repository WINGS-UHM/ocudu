#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

set(SOURCES
        cu_up_f1u_metrics_producer.cpp
        cu_up_f1u_metrics_consumers.cpp
        cu_up_pdcp_metrics_producer.cpp
        cu_up_pdcp_metrics_consumers.cpp)

add_library(ocudu_cu_up_unit_metrics_helpers STATIC ${SOURCES})
target_include_directories(ocudu_cu_up_unit_metrics_helpers PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(ocudu_cu_up_unit_metrics_helpers PRIVATE ocudu_cu_up_metrics_json_generators_helpers)
