#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

set_directory_properties(PROPERTIES LABELS "ecpri")

add_executable(ecpri_packet_builder_impl_test ecpri_packet_builder_test.cpp)
target_link_libraries(ecpri_packet_builder_impl_test ocudu_ofh_ecpri ocudu_support ocudulog gtest gtest_main)
gtest_discover_tests(ecpri_packet_builder_impl_test)

add_executable(ecpri_packet_decoder_impl_test ecpri_packet_decoder_impl_test.cpp)
target_link_libraries(ecpri_packet_decoder_impl_test ocudu_ofh_ecpri ocudu_support ocudulog gtest gtest_main)
gtest_discover_tests(ecpri_packet_decoder_impl_test)
