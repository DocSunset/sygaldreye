add_library(material INTERFACE)
target_include_directories(material INTERFACE .)
target_include_directories(material INTERFACE $ENV{EIGEN3_INCLUDE_DIR})
