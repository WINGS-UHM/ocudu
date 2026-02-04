#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

if (NOT DISABLE_MBEDTLS AND NOT DISABLE_SCTP)
    add_subdirectory(cu)
    add_subdirectory(cu_cp)
    add_subdirectory(cu_up)
    add_subdirectory(du)
    add_subdirectory(du_low)
    add_subdirectory(gnb)
endif ()
add_subdirectory(examples)
add_subdirectory(helpers)
add_subdirectory(services)
add_subdirectory(units)
