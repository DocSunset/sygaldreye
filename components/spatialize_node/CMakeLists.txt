add_library(spatialize_node STATIC spatialize_node.cpp)
target_include_directories(spatialize_node PUBLIC . $ENV{EIGEN3_INCLUDE_DIR})
target_link_libraries(spatialize_node PUBLIC sygaldry_endpoints hrtf_spatializer)
target_compile_options(spatialize_node PRIVATE -Wall -Wextra)
