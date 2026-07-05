add_library(syg_chunk chunk.cpp)
target_include_directories(syg_chunk PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(syg_chunk PUBLIC syg_cid syg_dagcbor)
