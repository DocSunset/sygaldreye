add_library(syg_executor regions.cpp)
target_include_directories(syg_executor PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(syg_executor PUBLIC syg_crown syg_naming syg_organs)
