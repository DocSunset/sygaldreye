add_library(hand_node INTERFACE)
target_include_directories(hand_node INTERFACE . $ENV{EIGEN3_INCLUDE_DIR})
target_link_libraries(hand_node INTERFACE sygaldry_endpoints)
