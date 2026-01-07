#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

set(SOURCES
        ecpri_factories.cpp
        ecpri_packet_builder_impl.cpp
        ecpri_packet_decoder_impl.cpp)

add_library(ocudu_ofh_ecpri STATIC ${SOURCES})
target_link_libraries(ocudu_ofh_ecpri ocudulog)
