add_library(render_nodes STATIC render_nodes.cpp)
target_include_directories(render_nodes PUBLIC . $ENV{EIGEN3_INCLUDE_DIR})
target_link_libraries(render_nodes PUBLIC gpu_texture gl_program sygaldry_endpoints)
target_compile_options(render_nodes PRIVATE -Wall -Wextra)
