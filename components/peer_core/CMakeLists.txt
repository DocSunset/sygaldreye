add_library(peer_core STATIC peer_core.cpp peer_core_http.cpp peer_core_bridge.cpp)
target_include_directories(peer_core PUBLIC .)
target_link_libraries(peer_core
    PUBLIC component_registry signal_graph subgraph_node http_server event_queue remote_node audio_region
    PRIVATE editor_node spawner_node fly_camera_node port_schema_reader)
target_compile_options(peer_core PRIVATE -Wall -Wextra)
