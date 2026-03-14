set(SOURCES isac_sink.cpp)

add_library(ocudu_isac STATIC ${SOURCES})
target_link_libraries(ocudu_isac ocudu_support)
add_to_exported_libs(ocudu_isac)