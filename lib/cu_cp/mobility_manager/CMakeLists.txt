#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

set(SOURCES
  mobility_manager_impl.cpp
  metrics/mobility_manager_metrics_aggregator.cpp
)

add_library(ocudu_cu_cp_mobility_manager STATIC ${SOURCES})
target_link_libraries(ocudu_cu_cp_mobility_manager ocudu_ran)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
