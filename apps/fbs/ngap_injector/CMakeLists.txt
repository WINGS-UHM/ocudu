# UHM WINGS Fake Base Station Research

add_executable(ngap_injector
    ngap_injector.cpp
)

target_include_directories(ngap_injector PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/external
)

target_link_libraries(ngap_injector PRIVATE
    asn1_utils
    ocudu_gateway
    ocudu_support
    ocudulog
    ngap_asn1
)

install(TARGETS ngap_injector
    RUNTIME
)
