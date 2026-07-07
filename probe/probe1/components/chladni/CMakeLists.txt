add_library(chladni STATIC chladni.cpp)
target_include_directories(chladni PUBLIC . $ENV{EIGEN3_INCLUDE_DIR})
target_link_libraries(chladni PUBLIC tri_mesh render_payloads sygaldry_endpoints)
target_compile_options(chladni PRIVATE -Wall -Wextra)
