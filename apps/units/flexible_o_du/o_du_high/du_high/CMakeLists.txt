#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

add_subdirectory(metrics)

set(SOURCES
        du_high_config_cli11_schema.cpp
        du_high_config_translators.cpp
        du_high_config_validator.cpp
        du_high_config_yaml_writer.cpp
        commands/du_high_remote_commands.cpp
        ntn/du_high_ntn_config_cli11_schema.cpp
        ntn/du_high_ntn_config_yaml_writer.h
        ntn/du_high_ntn_config_yaml_writer.cpp
        ntn/du_high_ntn_config_translators.h
        ntn/ntn_logger_registrator.h
        ntn/du_high_ntn_config_translators.cpp)

add_library(ocudu_du_high_unit_helpers STATIC ${SOURCES})
target_include_directories(ocudu_du_high_unit_helpers PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(ocudu_du_high_unit_helpers
        sched_config
        ocudu_app_helpers
        ocudu_cpu_affinities_helper
        ocudu_du_high_unit_metrics_helpers
        ocudu_pusch_processor)
