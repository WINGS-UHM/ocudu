#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

include_directories(../../..)

add_library(nrppa_test_doubles nrppa_test_message_validators.cpp nrppa_test_messages.cpp)
set_target_properties(nrppa_test_doubles PROPERTIES UNITY_BUILD ON)
target_link_libraries(nrppa_test_doubles ocudu_nrppa ocudu_support ocudulog nrppa_asn1)
