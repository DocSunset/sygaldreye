add_library(sprite INTERFACE)
target_include_directories(sprite INTERFACE .)
target_link_libraries(sprite INTERFACE eyeballs_node_abi render_payloads tri_mesh)
