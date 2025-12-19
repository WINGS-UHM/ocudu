#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

set(SOURCES
  rohc_lib_compressor.cpp
  rohc_lib_decompressor.cpp
  rohc_lib_factory.cpp
)

add_library(ocudu_rohc_lib STATIC ${SOURCES})
target_include_directories(ocudu_rohc_lib PUBLIC ${ROHC_INCLUDE_DIRS})
target_link_libraries(ocudu_rohc_lib ocudu_support ${ROHC_LIBRARIES})
