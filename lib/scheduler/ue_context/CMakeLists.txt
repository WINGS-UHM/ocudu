#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

set(SOURCES
        logical_channel_system.cpp
        ta_management_system.cpp
        ue_channel_state_manager.cpp
        ue_link_adaptation_controller.cpp
        ue_drx_controller.cpp
        ue_cell_repository.cpp
        ue_repository.cpp
        ue.cpp
        ue_cell.cpp)

add_library(sched_ue_context STATIC ${SOURCES})
target_link_libraries(sched_ue_context ocudu_ran sched_support sched_cell)
