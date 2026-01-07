#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

set(SOURCES
        nrppa_factory.cpp
        nrppa_helper.cpp
        nrppa_impl.cpp
        du_context/nrppa_du_context.cpp
        meas_context/nrppa_meas_context.cpp
        procedures/e_cid_measurement_initiation_procedure.cpp
        procedures/e_cid_measurement_termination_procedure.cpp
        procedures/measurement_procedure.cpp
        procedures/trp_information_exchange_procedure.cpp
        procedures/positioning_information_exchange_procedure.cpp
        procedures/positioning_activation_procedure.cpp
        ue_context/nrppa_ue_context.cpp
        )

add_library(ocudu_nrppa STATIC ${SOURCES})

target_link_libraries(ocudu_nrppa
	ocudu_cu_cp
	nrppa_asn1
	ocudu_support)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
