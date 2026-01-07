#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

add_executable(ocucp
        cu_cp.cpp
        cu_cp_appconfig_cli11_schema.cpp
)

install(TARGETS ocucp
        RUNTIME)

target_link_libraries(ocucp PRIVATE
        ocudu_app_helpers
        ocudu_app_services
        ocudu_e1_gateway
        ocudu_e2
        ocudu_f1c_gateway
        ocudu_ngap
        ocudu_o_cu_cp_app_unit
        ocudu_pcap
        ocudu_support
        ocudu_versioning
)

add_backward(ocucp)

target_include_directories(ocucp PRIVATE ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/external)
notify_binary_target(ocucp)
