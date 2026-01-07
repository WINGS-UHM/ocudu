#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

set(SOURCES
        backend_worker.cpp
        ocudulog.cpp
        ocudulog_c.cpp
        event_trace.cpp)

include_directories(${PROJECT_SOURCE_DIR}/include/ocudu/ocudulog/bundled/)
include_directories(${PROJECT_SOURCE_DIR}/include/ocudu/ocudulog/formatters)

set(SOURCES
        ${SOURCES}
        ${CMAKE_CURRENT_SOURCE_DIR}/formatters/json_formatter.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/formatters/text_formatter.cpp)


add_library(ocudulog STATIC ${SOURCES})
target_link_libraries(ocudulog fmt ${CMAKE_THREAD_LIBS_INIT})

add_to_exported_libs(ocudulog)
