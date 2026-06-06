add_library(input STATIC input.cpp)
target_include_directories(input PUBLIC . $ENV{EIGEN3_INCLUDE_DIR})
target_link_libraries(input PUBLIC openxr_loader PRIVATE log)
target_compile_options(input PRIVATE -Wall -Wextra)
