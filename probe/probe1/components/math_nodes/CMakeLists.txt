add_library(math_nodes INTERFACE)
target_include_directories(math_nodes INTERFACE .)
target_link_libraries(math_nodes INTERFACE sygaldry_endpoints)
