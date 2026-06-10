add_library(aurora_curtain STATIC aurora_curtain.cpp)
target_include_directories(aurora_curtain PUBLIC . $ENV{EIGEN3_INCLUDE_DIR})
target_link_libraries(aurora_curtain PUBLIC gl_geometry gl_program sygaldry_endpoints)
target_compile_options(aurora_curtain PRIVATE -Wall -Wextra)
