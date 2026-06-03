add_library(cube_geometry STATIC cube_geometry.cpp)
target_include_directories(cube_geometry PUBLIC .)
target_link_libraries(cube_geometry PRIVATE GLESv3 log)
target_compile_options(cube_geometry PRIVATE -Wall -Wextra)
