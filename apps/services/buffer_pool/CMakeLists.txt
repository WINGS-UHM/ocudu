#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

set(SOURCES
        buffer_pool_appconfig_cli11_schema.cpp
        buffer_pool_config_yaml_writer.cpp
        metrics/buffer_pool_metrics_builder.cpp)

add_library(ocudu_buffer_pool_app_service STATIC ${SOURCES})
target_include_directories(ocudu_buffer_pool_app_service PRIVATE ${CMAKE_SOURCE_DIR})
