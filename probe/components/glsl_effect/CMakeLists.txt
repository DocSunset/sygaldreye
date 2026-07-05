add_library(glsl_effect STATIC glsl_effect.cpp)
target_include_directories(glsl_effect PUBLIC .)
target_link_libraries(glsl_effect PUBLIC gpu_texture gl_program sygaldry_endpoints)
target_compile_options(glsl_effect PRIVATE -Wall -Wextra)
