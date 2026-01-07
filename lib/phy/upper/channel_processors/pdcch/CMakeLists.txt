#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

add_library(ocudu_pdcch_processor STATIC
        factories.cpp
        pdcch_encoder_impl.cpp
        pdcch_modulator_impl.cpp
        pdcch_processor_impl.cpp
        pdcch_processor_validator_impl.cpp)
target_link_libraries(ocudu_pdcch_processor ocudu_ran ocuduvec)
