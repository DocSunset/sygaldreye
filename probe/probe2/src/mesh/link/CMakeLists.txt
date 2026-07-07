add_library(syg_mesh_link link.cpp)
target_include_directories(syg_mesh_link PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(syg_mesh_link PUBLIC syg_mesh_identity PkgConfig::SODIUM)
