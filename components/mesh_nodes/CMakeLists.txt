add_library(mesh_nodes STATIC mesh_nodes.cpp)
target_include_directories(mesh_nodes PUBLIC . $ENV{EIGEN3_INCLUDE_DIR})
target_link_libraries(mesh_nodes PUBLIC eyeballs_node_abi tri_mesh gl_program gpu_texture sygaldry_endpoints)
target_compile_options(mesh_nodes PRIVATE -Wall -Wextra)
