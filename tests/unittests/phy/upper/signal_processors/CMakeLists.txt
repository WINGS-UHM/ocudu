#
# Copyright 2021-2025 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/test_data)
set(TEST_DATA_DIR ${CMAKE_CURRENT_BINARY_DIR}/test_data)

set_directory_properties(PROPERTIES LABELS "phy")

add_executable(pss_processor_test ssb/pss_processor_test.cpp)
target_link_libraries(pss_processor_test ocudu_ssb_generators ocuduvec ocudulog)
add_test(pss_processor_test pss_processor_test)

add_executable(sss_processor_test ssb/sss_processor_test.cpp)
target_link_libraries(sss_processor_test ocudu_ssb_generators ocuduvec ocudulog)
add_test(sss_processor_test sss_processor_test)

add_executable(prs_generator_validator_test prs/prs_generator_validator_test.cpp)
target_link_libraries(prs_generator_validator_test ocudu_prs_generator ocudu_ran ocuduvec ocudu_sequence_generators ocudu_channel_precoder ocudulog gtest gtest_main)
add_test(prs_generator_validator_test prs_generator_validator_test)

if (USE_PHY_TESTVECTORS)
    add_executable(prs_generator_vectortest prs/prs_generator_vectortest.cpp)
    target_link_libraries(prs_generator_vectortest ocudu_prs_generator ocudu_ran ocuduvec ocudu_sequence_generators ocudu_channel_precoder ocudulog gtest gtest_main)
    add_test_vector(prs_generator_vectortest prs/prs_generator_test_data.tar.gz "")

    add_executable(ptrs_pdsch_generator_vectortest ptrs/ptrs_pdsch_generator_vectortest.cpp)
    target_link_libraries(ptrs_pdsch_generator_vectortest ocudu_ptrs_generator ocudu_ran ocudu_phy_support
            ocuduvec ocudu_sequence_generators ocudu_channel_precoder ocudulog gtest gtest_main)
    add_test_vector(ptrs_pdsch_generator_vectortest ptrs/ptrs_pdsch_generator_test_data.tar.gz "")

    add_executable(srs_estimator_test
            srs/srs_estimator_logger_test.cpp
            srs/srs_estimator_validator_test.cpp
            srs/srs_estimator_vectortest.cpp)
    target_link_libraries(srs_estimator_test ocudu_srs_estimator ocudu_dft ocudu_phy_support ocuduvec ocudu_sequence_generators ocudulog gtest gtest_main)
    add_test_vector(srs_estimator_test srs/srs_estimator_test_data.tar.gz "")

    add_executable(dmrs_pbch_processor_test ssb/dmrs_pbch_processor_test.cpp)
    target_link_libraries(dmrs_pbch_processor_test ocudu_ssb_generators ocuduvec ocudu_sequence_generators ocudulog)
    add_test_vector(dmrs_pbch_processor_test ssb/dmrs_pbch_processor_test_data.tar.gz "")

    add_executable(dmrs_pdcch_processor_test pdcch/dmrs_pdcch_processor_test.cpp)
    target_link_libraries(dmrs_pdcch_processor_test ocudu_dmrs_pdcch ocudu_ran ocudu_phy_support ocuduvec ocudu_sequence_generators ocudulog ocudu_channel_precoder)
    add_test(dmrs_pdcch_processor_test dmrs_pdcch_processor_test)
    add_test_vector(dmrs_pdcch_processor_test pdcch/dmrs_pdcch_processor_test_data.tar.gz "")

    add_executable(dmrs_pdsch_processor_test pdsch/dmrs_pdsch_processor_test.cpp)
    target_link_libraries(dmrs_pdsch_processor_test ocudu_dmrs_pdsch ocudu_ran ocudu_phy_support ocuduvec ocudu_sequence_generators ocudulog ocudu_channel_precoder)
    add_test_vector(dmrs_pdsch_processor_test pdsch/dmrs_pdsch_processor_test_data.tar.gz "")

    add_executable(dmrs_pucch_estimator_test pucch/dmrs_pucch_estimator_test.cpp)
    target_link_libraries(dmrs_pucch_estimator_test ocudu_dmrs_pucch ocudu_channel_estimator ocudu_dft ocuduvec
        ocudu_sequence_generators ocudulog ocudu_support gtest gtest_main)
    add_test_vector(dmrs_pucch_estimator_test pucch/dmrs_pucch_estimator_test_data.tar.gz "")

    add_executable(dmrs_pusch_estimator_test pusch/dmrs_pusch_estimator_test.cpp)
    target_link_libraries(dmrs_pusch_estimator_test
        ocudu_dmrs_pusch
        ocudu_channel_estimator
        ocudu_dft
        ocudu_sequence_generators
        ocudu_phy_support
        ocudu_pusch_processor
        ocuduvec
        ocudulog
        gtest
        gtest_main
    )
    add_test_vector(dmrs_pusch_estimator_test pusch/dmrs_pusch_estimator_test_data.tar.gz "")

    add_executable(nzp_csi_rs_generator_test nzp_csi_rs/nzp_csi_rs_generator_test.cpp)
    target_link_libraries(nzp_csi_rs_generator_test ocudu_nzp_csi_rs_generator ocuduvec ocudu_sequence_generators ocudulog
            ocudu_upper_phy_support ocudu_phy_support gtest gtest_main ocudu_channel_precoder)
    add_test(nzp_csi_rs_generator_test nzp_csi_rs_generator_test)
    add_test_vector(nzp_csi_rs_generator_test nzp_csi_rs/nzp_csi_rs_generator_test_data.tar.gz "")

    add_executable(port_channel_estimator_test channel_estimator/port_channel_estimator_test.cpp)
    target_link_libraries(port_channel_estimator_test ocudu_channel_estimator ocudu_dft ocudulog gtest gtest_main)
    add_test_vector(port_channel_estimator_test channel_estimator/port_channel_estimator_test_data.tar.gz "")
endif (USE_PHY_TESTVECTORS)
