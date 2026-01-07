#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

add_executable(ocuup
        cu_up.cpp
        cu_up_appconfig_cli11_schema.cpp
        cu_up_appconfig_validator.cpp
)

install(TARGETS ocuup
        RUNTIME)

target_link_libraries(ocuup PRIVATE
        ocudu_app_helpers
        ocudu_app_services
        ocudu_e1_gateway
        ocudu_e2
        ocudu_f1u_cu_up_split_connector
        ocudu_o_cu_up_app_unit
        ocudu_pcap
        ocudu_support
        ocudu_versioning
)

add_backward(ocuup)

target_include_directories(ocuup PRIVATE ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/external)
notify_binary_target(ocuup)
