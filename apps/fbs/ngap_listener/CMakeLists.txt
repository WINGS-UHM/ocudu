# UHM WINGS Fake Base Station Research

add_executable(ngap_listener
    ngap_listener.cpp
)

target_include_directories(ngap_listener PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/external
)

target_link_libraries(ngap_listener PRIVATE
    asn1_utils
    ocudu_support
    ocudulog
    ngap_asn1
)

install(TARGETS ngap_listener
    RUNTIME
)
