#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

set_directory_properties(PROPERTIES LABELS "phy")

add_subdirectory(precoding)

add_executable(dft_processor_test dft_processor_test.cpp)
target_link_libraries(dft_processor_test ocudulog ocudu_dft gtest gtest_main)
add_test(dft_processor_test dft_processor_test)

if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "x86_64")
    add_executable(dft_processor_ci16_test dft_processor_ci16_test.cpp)
    target_link_libraries(dft_processor_ci16_test ocudulog ocudu_dft gtest gtest_main)
    add_test(dft_processor_ci16_test dft_processor_ci16_test)
endif (${CMAKE_SYSTEM_PROCESSOR} MATCHES "x86_64")

if ((ENABLE_MKL AND MKL_FOUND) OR (ENABLE_ARMPL AND ARMPL_FOUND) OR (ENABLE_FFTW AND FFTW3F_FOUND))
    target_compile_definitions(dft_processor_test PRIVATE HAVE_FFTW)
endif ((ENABLE_MKL AND MKL_FOUND) OR (ENABLE_ARMPL AND ARMPL_FOUND) OR (ENABLE_FFTW AND FFTW3F_FOUND))

if (ENABLE_FFTZ AND FFTZ_FOUND)
    target_compile_definitions(dft_processor_test PRIVATE HAVE_FFTZ)
endif (ENABLE_FFTZ AND FFTZ_FOUND)
