#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

set(SOURCES
        du_stop_procedure.cpp
        du_cell_stop_procedure.cpp
        du_ue_ric_configuration_procedure.cpp
        du_param_config_procedure.cpp
        du_mac_ntn_param_update_procedure.cpp
        du_setup_procedure.cpp
        ue_configuration_procedure.cpp
        ue_creation_procedure.cpp
        ue_deletion_procedure.cpp
        du_ue_reset_procedure.cpp
        cu_configuration_procedure.cpp
        f1c_disconnection_handling_procedure.cpp
        du_positioning_measurement_procedure.cpp
        du_ue_positioning_info_procedure.cpp
)
add_library(ocudu_du_manager_procedures STATIC ${SOURCES})
target_link_libraries(ocudu_du_manager_procedures sched_config ocudu_ran mac_configuration_helpers du_ue ocudu_du_manager)
