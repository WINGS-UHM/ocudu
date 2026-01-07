#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

add_definitions(-DUWS_NO_ZLIB)

set(SOURCES
        remote_control_appconfig_cli11_schema.cpp
        remote_server.cpp)

add_library(ocudu_remote_control_app_service STATIC ${SOURCES})
target_include_directories(ocudu_remote_control_app_service PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(ocudu_remote_control_app_service PRIVATE uSockets)
