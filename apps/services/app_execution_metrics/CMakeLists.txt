#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

set(SOURCES
        executor_metrics_config_cli11_schema.cpp
        executor_metrics_config_yaml_writer.cpp
        metrics/app_execution_metrics_builder.cpp)

add_library(ocudu_exec_metrics_app_service STATIC ${SOURCES})
target_include_directories(ocudu_exec_metrics_app_service PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(ocudu_exec_metrics_app_service PRIVATE ocudu_metrics_app_helpers ocudu_metrics_json_generators)
