add_library(visual_node INTERFACE)
target_include_directories(visual_node INTERFACE ${CMAKE_CURRENT_SOURCE_DIR} $ENV{EIGEN3_INCLUDE_DIR})
target_link_libraries(visual_node INTERFACE sygaldry_endpoints)
