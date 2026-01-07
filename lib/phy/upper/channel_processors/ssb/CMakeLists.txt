#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

add_library(ocudu_ssb_tx_processor STATIC
        factories.cpp
        pbch_encoder_impl.cpp
        pbch_modulator_impl.cpp
        ssb_processor_impl.cpp)
target_link_libraries(ocudu_ssb_tx_processor
        ocudu_sequence_generators
        ocudu_signal_processors)