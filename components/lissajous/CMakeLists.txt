add_library(lissajous STATIC lissajous.cpp)
target_include_directories(lissajous PUBLIC . $ENV{EIGEN3_INCLUDE_DIR})
target_link_libraries(lissajous PUBLIC tri_mesh render_payloads sygaldry_endpoints)
target_compile_options(lissajous PRIVATE -Wall -Wextra)
