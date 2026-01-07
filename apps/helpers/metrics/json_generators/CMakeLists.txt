#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

add_subdirectory(cu_cp)
add_subdirectory(cu_up)
add_subdirectory(du_high)
add_subdirectory(ru)

set(SOURCES
        executors.cpp
        o_du_high.cpp
        o_du_low.cpp
        resource_usage.cpp)

add_library(ocudu_metrics_json_generators STATIC ${SOURCES})
target_include_directories(ocudu_metrics_json_generators PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(ocudu_metrics_json_generators PRIVATE ocudu_cu_cp_metrics_json_generators_helpers
                                                             ocudu_cu_up_metrics_json_generators_helpers
                                                             ocudu_du_high_metrics_json_generators_helpers
                                                             ocudu_ru_metrics_json_generators_helpers)
