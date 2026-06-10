add_library(net_nodes STATIC net_nodes.cpp)
target_include_directories(net_nodes PUBLIC .)
target_link_libraries(net_nodes PUBLIC sygaldry_endpoints)
target_compile_options(net_nodes PRIVATE -Wall -Wextra -Wpedantic)
