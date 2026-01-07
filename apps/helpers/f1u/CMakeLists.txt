#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

set(SOURCES f1u_cli11_schema.cpp
            f1u_appconfig_validator.cpp
            f1u_config_yaml_writer.cpp
)

add_library(ocudu_f1u_app_helpers STATIC ${SOURCES})
target_include_directories(ocudu_f1u_app_helpers PRIVATE ${CMAKE_SOURCE_DIR})
