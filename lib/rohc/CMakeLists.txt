#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

set(SOURCES
  rohc_factory.cpp
  rohc_support.cpp
)

add_library(ocudu_rohc STATIC ${SOURCES})

set(OCUDU_ROHC_DEFINITIONS "")
set(OCUDU_ROHC_LIBRARIES "")

if(ROHC_FOUND)
  add_subdirectory(rohc_lib)
  list(APPEND OCUDU_ROHC_DEFINITIONS -DENABLE_ROHC_LIB)
  list(APPEND OCUDU_ROHC_LIBRARIES ocudu_rohc_lib)
endif()

add_definitions(${OCUDU_ROHC_DEFINITIONS})

target_link_libraries(ocudu_rohc ${OCUDU_ROHC_LIBRARIES})
