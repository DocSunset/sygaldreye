add_library(peer_core STATIC peer_core.cpp peer_core_http.cpp peer_core_bridge.cpp)
target_include_directories(peer_core PUBLIC .)
target_link_libraries(peer_core
    PUBLIC component_registry signal_graph subgraph_node http_server event_queue remote_node audio_region editor_layout
    PRIVATE spawner_node graph_source edit_sink fly_camera_node port_schema_reader
            handle_picker wire_drag slider_drag dwell_delete undo_node palette
            card_widgets_mesh card_labels_mesh editor_wires palette_mesh)
target_compile_options(peer_core PRIVATE -Wall -Wextra)
