#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

add_subdirectory(metrics)

set(SOURCES
        cu_cp_unit_config_cli11_schema.cpp
        cu_cp_config_translators.cpp
        cu_cp_unit_config_validator.cpp
        cu_cp_unit_config_yaml_writer.cpp)

add_library(ocudu_cu_cp_app_unit STATIC ${SOURCES})
target_include_directories(ocudu_cu_cp_app_unit PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(ocudu_cu_cp_app_unit PRIVATE ocudu_cu_cp_unit_metrics_helpers ocudu_app_helpers)
