add_library(cube_mesh STATIC cube_mesh.cpp)
target_include_directories(cube_mesh PUBLIC .)
target_include_directories(cube_mesh PUBLIC $ENV{EIGEN3_INCLUDE_DIR})
# gl_program headers are exposed via cube_mesh.hpp; propagate the include path
# even though the library linkage is private. Remove when header isolation lands.
target_include_directories(cube_mesh PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../gl_program)
target_link_libraries(cube_mesh PRIVATE GLESv3 log gl_program)
target_compile_options(cube_mesh PRIVATE -Wall -Wextra)
