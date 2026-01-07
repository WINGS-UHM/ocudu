#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

add_subdirectory(cbs)
add_subdirectory(converters)
add_subdirectory(du_ue)
add_subdirectory(procedures)
add_subdirectory(ran_resource_management)

set(SOURCES
        du_cell_manager.cpp
        du_manager_factory.cpp
        du_manager_impl.cpp
        du_positioning_handler_factory.cpp
        du_metrics_aggregator_impl.cpp
        du_positioning_manager_impl.cpp
)

add_library(ocudu_du_manager STATIC ${SOURCES})
target_link_libraries(ocudu_du_manager
        du_ue
        du_resource_manager
        du_manager_converters
        ocudu_du_config_validators
        mac_configuration_helpers
        ocudu_rlc
        ocudu_f1u_du
        ocudu_gtpu)
