add_library(wire_mesh STATIC wire_mesh.cpp)
target_include_directories(wire_mesh PUBLIC . $ENV{EIGEN3_INCLUDE_DIR})
target_link_libraries(wire_mesh PUBLIC gl_program sygaldry_endpoints)
target_compile_options(wire_mesh PRIVATE -Wall -Wextra)
