#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

set(SOURCES
  rrc_du_factory.cpp
  rrc_du_impl.cpp
  metrics/rrc_du_metrics_aggregator.cpp
  ue/meas_types_validators.cpp
  ue/rrc_asn1_helpers.cpp
  ue/rrc_asn1_converters.cpp
  ue/rrc_measurement_types_asn1_converters.cpp
  ue/rrc_ue_impl.cpp
  ue/rrc_ue_context.cpp
  ue/rrc_ue_message_handlers.cpp
  ue/rrc_ue_message_senders.cpp
  ue/rrc_ue_helpers.cpp
  ue/procedures/rrc_setup_procedure.cpp
  ue/procedures/rrc_reconfiguration_procedure.cpp
  ue/procedures/rrc_ue_capability_transfer_procedure.cpp
  ue/procedures/rrc_reestablishment_procedure.cpp
  ue/procedures/rrc_resume_procedure.cpp
)

add_library(ocudu_rrc STATIC ${SOURCES})
target_link_libraries(ocudu_rrc ocudu_up_resource_manager ocudu_ue_security_manager ocudu_security ocudu_pdcp rrc_nr_asn1)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
