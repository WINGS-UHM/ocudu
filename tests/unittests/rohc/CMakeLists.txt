#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

set_directory_properties(PROPERTIES LABELS "rohc")

add_executable(rohc_config_test rohc_config_test.cpp)
target_link_libraries(rohc_config_test ocudu_rohc ocudu_support ocudulog gtest gtest_main)
gtest_discover_tests(rohc_config_test)

add_executable(rohc_factory_test rohc_factory_test.cpp)
target_link_libraries(rohc_factory_test ocudu_rohc ocudu_support ocudulog gtest gtest_main)
gtest_discover_tests(rohc_factory_test)

if(ROHC_FOUND)
  add_executable(rohc_lib_compressor_decompressor_test rohc_lib_compressor_decompressor_test.cpp)
  target_include_directories(rohc_lib_compressor_decompressor_test PRIVATE ${CMAKE_SOURCE_DIR})
  target_link_libraries(rohc_lib_compressor_decompressor_test ocudu_rohc ocudu_support ocudulog gtest gtest_main)
  gtest_discover_tests(rohc_lib_compressor_decompressor_test)
endif()
