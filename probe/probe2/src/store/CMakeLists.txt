add_library(syg_store store.cpp)
target_include_directories(syg_store PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(syg_store PUBLIC syg_cid syg_dagcbor)
