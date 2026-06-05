add_library(text_mesh STATIC
    text_mesh.cpp
    dejavu_sans_metrics.cpp
    dejavu_sans_png.cpp
)
target_include_directories(text_mesh PUBLIC .)
target_include_directories(text_mesh PUBLIC $ENV{EIGEN3_INCLUDE_DIR})
target_link_libraries(text_mesh PUBLIC gl_program)
target_link_libraries(text_mesh PRIVATE log)
target_compile_options(text_mesh PRIVATE -Wall -Wextra)
