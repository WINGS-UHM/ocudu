#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

set(SOURCES
        xnap_impl.cpp
        xnap_asn1_utils.cpp
        log_helpers.cpp
        procedures/xn_setup_procedure.cpp
    )


add_library(ocudu_xnap STATIC ${SOURCES})

target_link_libraries(ocudu_xnap
	ocudu_cu_cp
	xnap_asn1
	ocudu_support)
