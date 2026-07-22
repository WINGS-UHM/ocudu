# UHM WINGS Fake Base Station Research

if (NOT DISABLE_SCTP)
    add_subdirectory(ngap_listener)
    add_subdirectory(ngap_injector)
    add_subdirectory(ngap_proxy)
endif ()
