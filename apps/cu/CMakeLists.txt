#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

add_executable(ocu
        cu.cpp
        cu_appconfig_cli11_schema.cpp
        cu_appconfig_validator.cpp
        cu_appconfig_yaml_writer.cpp
)

install(TARGETS ocu
        RUNTIME)

target_link_libraries(ocu PRIVATE
        ocudu_app_helpers
        ocudu_app_services
        ocudu_e1_gateway
        ocudu_e2
        ocudu_f1c_gateway
        ocudu_f1u_cu_up_split_connector
        ocudu_ngap
        ocudu_o_cu_cp_app_unit
        ocudu_o_cu_up_app_unit
        ocudu_pcap
        ocudu_support
        ocudu_versioning
        ngap_asn1
)

add_backward(ocu)

target_include_directories(ocu PRIVATE ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/external)
notify_binary_target(ocu)
