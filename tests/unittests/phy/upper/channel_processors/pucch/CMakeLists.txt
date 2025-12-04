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

add_executable(pucch_processor_format1_unittest pucch_processor_format1_unittest.cpp)
target_link_libraries(pucch_processor_format1_unittest
        ocudu_channel_equalizer
        ocudu_channel_processors
        ocudu_dft
        ocudu_transform_precoding
        ocudulog
        gtest
        gtest_main)
add_test(pucch_processor_format1_unittest pucch_processor_format1_unittest)

add_executable(pucch_processor_validators_test
        pucch_processor_validator_format0_test.cpp
        pucch_processor_validator_format1_test.cpp
        pucch_processor_validator_format2_test.cpp
        pucch_processor_validator_format3_test.cpp
        pucch_processor_validator_format4_test.cpp)
target_link_libraries(pucch_processor_validators_test
        ocudu_channel_equalizer
        ocudu_channel_processors
        ocudu_dft
        ocudu_phy_support
        ocudu_transform_precoding
        ocudu_upper_phy_support
        ocudulog
        gtest
        gtest_main)
add_test(pucch_processor_validators_test pucch_processor_validators_test)

if (USE_PHY_TESTVECTORS)
    add_executable(pucch_demodulator_format2_test pucch_demodulator_format2_test.cpp)
    target_link_libraries(pucch_demodulator_format2_test
            ocudu_channel_equalizer
            ocudu_channel_processors
            ocudu_dft
            ocudu_phy_support
            ocudu_transform_precoding
            ocudu_upper_phy_support
            ocudulog
            gtest
            gtest_main)
    add_test_vector(pucch_demodulator_format2_test pucch_demodulator_format2_test_data.tar.gz "")

    add_executable(pucch_demodulator_format3_test pucch_demodulator_format3_test.cpp)
    target_link_libraries(pucch_demodulator_format3_test
            ocudu_channel_equalizer
            ocudu_channel_processors
            ocudu_dft
            ocudu_phy_support
            ocudu_transform_precoding
            ocudu_upper_phy_support
            ocudulog
            gtest
            gtest_main)
    add_test_vector(pucch_demodulator_format3_test pucch_demodulator_format3_test_data.tar.gz "")

    add_executable(pucch_demodulator_format4_test pucch_demodulator_format4_test.cpp)
    target_link_libraries(pucch_demodulator_format4_test
            ocudu_channel_equalizer
            ocudu_channel_processors
            ocudu_dft
            ocudu_phy_support
            ocudu_transform_precoding
            ocudu_upper_phy_support
            ocudulog
            gtest
            gtest_main)
    add_test_vector(pucch_demodulator_format4_test pucch_demodulator_format4_test_data.tar.gz "")

    add_executable(pucch_processor_format0_vectortest pucch_processor_format0_vectortest.cpp)
    target_link_libraries(pucch_processor_format0_vectortest
            ocudu_channel_equalizer
            ocudu_channel_processors
            ocudu_dft
            ocudu_phy_support
            ocudu_transform_precoding
            ocudulog
            gtest
            gtest_main)
    add_test_vector(pucch_processor_format0_vectortest pucch_processor_format0_test_data.tar.gz "")

    add_executable(pucch_processor_format1_vectortest pucch_processor_format1_vectortest.cpp)
    target_link_libraries(pucch_processor_format1_vectortest
            ocudu_channel_equalizer
            ocudu_channel_processors
            ocudu_dft
            ocudu_phy_support
            ocudu_transform_precoding
            ocudulog
            gtest
            gtest_main)
    add_test_vector(pucch_processor_format1_vectortest pucch_processor_format1_test_data.tar.gz "")

    add_executable(pucch_processor_format2_vectortest pucch_processor_format2_vectortest.cpp)
    target_link_libraries(pucch_processor_format2_vectortest
            ocudu_channel_processors
            ocudu_channel_equalizer
            ocudu_dft
            ocudu_phy_support
            ocudu_transform_precoding
            gtest
            gtest_main)
    add_test(pucch_processor_format2_vectortest pucch_processor_format2_vectortest)
    add_test_vector(pucch_processor_format2_vectortest pucch_processor_format2_test_data.tar.gz "")

    add_executable(pucch_processor_format3_vectortest pucch_processor_format3_vectortest.cpp)
    target_link_libraries(pucch_processor_format3_vectortest
            ocudu_channel_processors
            ocudu_channel_equalizer
            ocudu_dft
            ocudu_phy_support
            ocudu_transform_precoding
            gtest
            gtest_main)
    add_test(pucch_processor_format3_vectortest pucch_processor_format3_vectortest)
    add_test_vector(pucch_processor_format3_vectortest pucch_processor_format3_test_data.tar.gz "")

    add_executable(pucch_processor_format4_vectortest pucch_processor_format4_vectortest.cpp)
    target_link_libraries(pucch_processor_format4_vectortest
            ocudu_channel_processors
            ocudu_channel_equalizer
            ocudu_dft
            ocudu_phy_support
            ocudu_transform_precoding
            gtest
            gtest_main)
    add_test(pucch_processor_format4_vectortest pucch_processor_format4_vectortest)
    add_test_vector(pucch_processor_format4_vectortest pucch_processor_format4_test_data.tar.gz "")
endif (USE_PHY_TESTVECTORS)
