set(NDK_GLUE $ENV{ANDROID_NDK_ROOT}/sources/android/native_app_glue)

add_library(native_app_glue STATIC ${NDK_GLUE}/android_native_app_glue.c)
target_include_directories(native_app_glue PUBLIC ${NDK_GLUE})

add_library(eyeballs SHARED app.cpp xr.cpp)
target_link_libraries(eyeballs PRIVATE native_app_glue android log openxr_loader renderer xr_session frame_loop scene input vr_math text_mesh text_label water_surface sky_dome star_field scene_snapshot aurora_curtain math_nodes net_nodes hand_node fly_camera_node spawner_node graph_source edit_sink card_mesh card_widgets_mesh card_labels_mesh editor_wires handle_picker wire_drag slider_drag dwell_delete undo_node palette palette_mesh peer_core mdns_advertiser param_registry mic_capture mic_input push_to_talk wav_player sample_player osc_node dac_node ugens spatialize_node spectrogram eyeballs_node_abi component_registry signal_graph subgraph_node wire_mesh xr_sources renderer_node lissajous chladni terrain_generator particle_system reaction_diffusion sun_light trigger_edge ptt_gate cube_node speech_to_text stt_whisper mesh_nodes render_region color_mesh vertex_color_mesh sprite tree_generator poke_stick poke_button ui_nodes rubber_band_controller flat_mesh)
target_include_directories(eyeballs PRIVATE ${CMAKE_BINARY_DIR}/generated)
target_compile_options(eyeballs PRIVATE -Wall -Wextra)
target_link_options(eyeballs PRIVATE -u ANativeActivity_onCreate)
