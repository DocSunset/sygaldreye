add_library(fly_camera STATIC fly_camera.cpp)
target_include_directories(fly_camera PUBLIC . $ENV{EIGEN3_INCLUDE_DIR})
target_compile_options(fly_camera PRIVATE -Wall -Wextra -Wpedantic)
