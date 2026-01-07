#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

add_subdirectory(du_manager)
add_subdirectory(test_mode)

add_library(ocudu_du_high STATIC
        du_high_impl.cpp
        du_high_factory.cpp
        du_high_clock_controller_factory.cpp
        du_high_executor_mapper.cpp)
target_link_libraries(ocudu_du_high PUBLIC ocudu_du_high_adapters)

add_library(ocudu_o_du_high STATIC
        o_du_high_factory.cpp
        o_du_high_impl.cpp
        o_du_high_metrics_notifier_proxy.cpp)
target_link_libraries(ocudu_o_du_high PUBLIC ocudu_du_config_validators
                                              ocudu_du_high
                                              ocudu_mac_fapi_fastpath_adaptor
                                              ocudu_fapi ocudu_e2)
