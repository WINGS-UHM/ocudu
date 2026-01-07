#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

set(SOURCES
        app_resource_usage.cpp
        app_resource_usage_config_cli11_schema.cpp
        app_resource_usage_config_yaml_writer.cpp
        metrics/app_resource_usage_metrics_consumer.cpp)

add_library(ocudu_resource_usage_app_service STATIC ${SOURCES})
target_include_directories(ocudu_resource_usage_app_service PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(ocudu_resource_usage_app_service PRIVATE ocudu_metrics_app_helpers ocudu_metrics_json_generators)
