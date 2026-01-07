#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

add_subdirectory(metrics)

set(SOURCES
        flexible_o_du_factory.cpp
        flexible_o_du_impl.cpp
        flexible_o_du_metrics_aggregator.cpp
        flexible_o_du_ntn_configuration_manager_factory.cpp
        commands/ntn_config_update_remote_command_factory.cpp)

add_library(ocudu_flexible_o_du_helpers STATIC ${SOURCES})
target_include_directories(ocudu_flexible_o_du_helpers PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(ocudu_flexible_o_du_helpers
        ocudu_o_du
        ocudu_pcap
        ocudu_app_services
        ocudu_o_du_low_unit_helpers
        ocudu_o_du_high_unit_helpers
        ocudu_flexible_o_du_unit_metrics_helpers)
