#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

set(SOURCES nru_packing.cpp)

add_library(ocudu_nru STATIC ${SOURCES})
target_link_libraries(ocudu_nru ocudu_support)
