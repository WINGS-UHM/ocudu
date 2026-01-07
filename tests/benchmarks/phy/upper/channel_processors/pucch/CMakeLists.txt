#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

add_executable(pucch_processor_benchmark pucch_processor_benchmark.cpp)
set(PUCCH_PROCESSOR_LIBRARIES
        ocudu_pucch_processor
        ocudu_channel_equalizer
        ocudu_channel_processors
        ocudu_transform_precoding
        ocudulog
        ocuduvec)

target_link_libraries(pucch_processor_benchmark ${PUCCH_PROCESSOR_LIBRARIES})
add_test(pucch_processor_benchmark pucch_processor_benchmark -m silent -R 1 -B 1 -T 2)
