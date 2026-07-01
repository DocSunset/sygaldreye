add_library(render_region STATIC render_region.cpp)
target_include_directories(render_region PUBLIC . $ENV{EIGEN3_INCLUDE_DIR})
target_link_libraries(render_region PUBLIC render_payloads gl_program gl_geometry tri_mesh)
target_compile_options(render_region PRIVATE -Wall -Wextra)
