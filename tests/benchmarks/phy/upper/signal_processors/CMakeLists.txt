#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

add_executable(dmrs_pdsch_processor_benchmark dmrs_pdsch_processor_benchmark.cpp)
target_link_libraries(dmrs_pdsch_processor_benchmark
        ocuduvec
        ocudulog
        ocudu_dmrs_pdsch
        ocudu_phy_support
        ocudu_sequence_generators
        ocudu_channel_precoder)
add_test(dmrs_pdsch_processor_benchmark dmrs_pdsch_processor_benchmark -D -s -R 1)


add_executable(srs_estimator_benchmark srs_estimator_benchmark.cpp)
target_link_libraries(srs_estimator_benchmark
        ocuduvec
        ocudulog
        ocudu_channel_precoder
        ocudu_srs_estimator
        ocudu_phy_support
        ocudu_dft
        ocudu_sequence_generators)
add_test(srs_estimator_benchmark srs_estimator_benchmark -D -s -R 1)
