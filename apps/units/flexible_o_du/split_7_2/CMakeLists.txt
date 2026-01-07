#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#


add_subdirectory(helpers)

# Build the flexible du split 7.2 library when it is selected in the definition.
set(SOURCES
        split_7_2_o_du_application_unit_impl.cpp
        split_7_2_o_du_factory.cpp
        split_7_2_o_du_unit_cli11_schema.cpp
        split_7_2_o_du_unit_config_validator.cpp
        split_7_2_o_du_unit_config_yaml_writer.cpp)

add_library(ocudu_flexible_o_du_split_7_2 STATIC ${SOURCES})
target_include_directories(ocudu_flexible_o_du_split_7_2 PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(ocudu_flexible_o_du_split_7_2
        ocudu_split_7_2_app_unit_helpers
        ocudu_flexible_o_du_helpers)
