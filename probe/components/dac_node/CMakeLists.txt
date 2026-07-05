add_library(dac_node INTERFACE)
target_include_directories(dac_node INTERFACE .)
target_link_libraries(dac_node INTERFACE sygaldry_endpoints)
