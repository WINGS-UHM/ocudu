# UHM WINGS Fake Base Station Research

add_executable(ngap_proxy
    ngap_proxy.cpp
)

target_include_directories(ngap_proxy PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/external
)

target_link_libraries(ngap_proxy PRIVATE
    asn1_utils
    ocudu_gateway
    ocudu_support
    ocudulog
    ngap_asn1
    rrc_nr_asn1
)

install(TARGETS ngap_proxy
    RUNTIME
)
