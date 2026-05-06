# UHM WINGS Fake Base Station Research

set(NGAP_INJECTOR_CORE_SOURCES
    config.cpp
    negative_tests.cpp
    ngap_decoder.cpp
    pcap_parser.cpp
    sctp_transport.cpp
    sniffer.cpp
    state_machine.cpp
)

add_library(ngap_injector_core STATIC ${NGAP_INJECTOR_CORE_SOURCES})
target_include_directories(ngap_injector_core PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/external
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(ngap_injector_core PUBLIC
    asn1_utils
    ocudu_gateway
    ocudu_ngap
    ocudu_support
    ocudulog
    ngap_asn1
    ocudu::yaml-cpp
)

add_executable(ngap_injector
    ngap_injector.cpp
)

target_include_directories(ngap_injector PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/external
)

target_link_libraries(ngap_injector PRIVATE
    ngap_injector_core
)

install(TARGETS ngap_injector
    RUNTIME
)
