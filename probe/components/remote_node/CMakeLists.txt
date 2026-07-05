add_library(remote_node STATIC remote_node.cpp remote_descriptor.cpp)
target_include_directories(remote_node PUBLIC .)
target_link_libraries(remote_node PUBLIC ws_link eyeballs_node_abi port_schema_reader)
target_compile_options(remote_node PRIVATE -Wall -Wextra)
