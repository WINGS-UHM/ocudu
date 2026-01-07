#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#


set(SOURCES
        o_cu_cp_e2_config_cli11_schema.cpp
        o_cu_cp_e2_config_translators.cpp
        o_cu_cp_e2_config_yaml_writer.cpp)

add_library(ocudu_o_cu_cp_e2_app_unit STATIC ${SOURCES})
target_include_directories(ocudu_o_cu_cp_e2_app_unit PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(ocudu_o_cu_cp_e2_app_unit ocudu_e2_app_helpers)
