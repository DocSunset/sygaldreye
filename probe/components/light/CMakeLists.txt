add_library(light INTERFACE)
target_include_directories(light INTERFACE .)
target_include_directories(light INTERFACE $ENV{EIGEN3_INCLUDE_DIR})
