add_library(reaction_diffusion STATIC reaction_diffusion.cpp)
target_include_directories(reaction_diffusion PUBLIC . $ENV{EIGEN3_INCLUDE_DIR})
target_link_libraries(reaction_diffusion PUBLIC tri_mesh render_payloads sygaldry_endpoints)
target_compile_options(reaction_diffusion PRIVATE -Wall -Wextra)
