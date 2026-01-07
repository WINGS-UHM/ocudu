#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

set(SOURCES
  rlc_am_pdu.cpp
  rlc_bearer_metrics_collector.cpp
  rlc_factory.cpp
  rlc_tx_am_entity.cpp
  rlc_tx_tm_entity.cpp
  rlc_tx_um_entity.cpp
  rlc_rx_am_entity.cpp
  rlc_rx_tm_entity.cpp
  rlc_rx_um_entity.cpp
)

add_library(ocudu_rlc STATIC ${SOURCES})
target_link_libraries(ocudu_rlc ocudu_instrumentation ocudu_support ocudu_pcap)
