add_library(ugens INTERFACE)
target_include_directories(ugens INTERFACE .)
target_link_libraries(ugens INTERFACE sygaldry_endpoints synth_core biquad_filter)
