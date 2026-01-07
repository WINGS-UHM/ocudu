#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

set_directory_properties(PROPERTIES LABELS "phy")

add_executable(ofdm_modulator_unittest ofdm_modulator_unittest.cpp)
target_link_libraries(ofdm_modulator_unittest ocudulog ocudu_lower_phy_modulation)
add_test(ofdm_modulator_unittest ofdm_modulator_unittest)

add_executable(ofdm_demodulator_unittest ofdm_demodulator_unittest.cpp)
target_link_libraries(ofdm_demodulator_unittest ocudulog ocudu_lower_phy_modulation)
add_test(ofdm_demodulator_unittest ofdm_demodulator_unittest)
