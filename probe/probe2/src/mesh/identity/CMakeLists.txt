add_library(syg_mesh_identity identity.cpp)
target_include_directories(syg_mesh_identity PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(syg_mesh_identity PUBLIC syg_cid PkgConfig::SODIUM)
