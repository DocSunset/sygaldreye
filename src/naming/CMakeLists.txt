add_library(syg_naming environment/environment.cpp oracle/oracle.cpp resolver/resolver.cpp)
target_include_directories(syg_naming PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(syg_naming PUBLIC syg_address syg_cid syg_dagcbor)
