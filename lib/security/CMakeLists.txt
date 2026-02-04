#
# Copyright 2021-2026 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#


# Crypto
find_package(MbedTLS REQUIRED)
if (MBEDTLS_FOUND)
    set(SEC_INCLUDE_DIRS "${MBEDTLS_INCLUDE_DIRS}")
    if(BUILD_STATIC)
        set(SEC_LIBRARIES "${MBEDTLS_STATIC_LIBRARIES}")
    else(BUILD_STATIC)
        set(SEC_LIBRARIES "${MBEDTLS_LIBRARIES}")
    endif(BUILD_STATIC)
    sbom_add(
            PACKAGE            "mbedTLS"
            DOWNLOAD_LOCATION  "https://tls.mbed.org"
            SUPPLIER           "MbedTLS project"
            LICENSE            "Apache-2.0"
            VERSION            ${PC_MBEDTLS_VERSION}
    )
else(MBEDTLS_FOUND)
    message(FATAL_ERROR "mbedTLS is required to build ${CMAKE_PROJECT_NAME}")
endif (MBEDTLS_FOUND)

set(SOURCES
  ciphering_engine_nea1.cpp
  ciphering_engine_nea2.cpp
  ciphering_engine_nea3.cpp
  integrity_engine_generic.cpp
  integrity_engine_nia2_cmac.cpp
  integrity_engine_nia2_non_cmac.cpp
  security.cpp
  security_engine_impl.cpp
  s3g.cpp
  zuc.cpp
)

add_library(ocudu_security STATIC ${SOURCES})
target_include_directories(ocudu_security PUBLIC ${SEC_INCLUDE_DIRS})
target_link_libraries(ocudu_security ${SEC_LIBRARIES})
