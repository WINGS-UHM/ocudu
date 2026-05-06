# UHM WINGS Fake Base Station Research

add_executable(ngap_injector_test
    ngap_injector_test.cpp
)

target_include_directories(ngap_injector_test PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/external
    ${CMAKE_SOURCE_DIR}/apps/fbs/ngap_injector
)

target_link_libraries(ngap_injector_test
    ngap_injector_core
    GTest::gtest
    ocudu_gtest_main
)

gtest_discover_tests(ngap_injector_test PROPERTIES LABELS "ngap_injector")
