add_library(osc_node INTERFACE)
target_include_directories(osc_node INTERFACE .)
target_link_libraries(osc_node INTERFACE sygaldry_endpoints synth_core)
