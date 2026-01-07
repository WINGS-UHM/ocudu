#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

set(SOURCES
        ngap_metrics_consumers.cpp
        rrc_metrics_consumers.cpp
)

add_library(ocudu_cu_cp_unit_metrics_consumers STATIC ${SOURCES})
target_include_directories(ocudu_cu_cp_unit_metrics_consumers PRIVATE ${CMAKE_SOURCE_DIR})
