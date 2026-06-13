set(NDK_GLUE $ENV{ANDROID_NDK_ROOT}/sources/android/native_app_glue)

add_library(native_app_glue STATIC ${NDK_GLUE}/android_native_app_glue.c)
target_include_directories(native_app_glue PUBLIC ${NDK_GLUE})

add_library(eyeballs SHARED app.cpp xr.cpp)
target_link_libraries(eyeballs PRIVATE native_app_glue android log openxr_loader renderer xr_session frame_loop scene input vr_math cube_mesh text_mesh text_label water_surface sky_dome star_field scene_snapshot aurora_curtain math_nodes net_nodes ui_nodes hand_node fly_camera_node spawner_node peer_core mdns_advertiser param_registry mic_capture mic_input push_to_talk wav_player sample_player osc_node dac_node ugens spatialize_node poke_stick poke_button spectrogram eyeballs_node_abi component_registry signal_graph subgraph_node editor_node wire_mesh vr_panel ray_selector rd_gpu xr_sources renderer_node lissajous chladni terrain_generator particle_system reaction_diffusion sun_light trigger_edge ptt_gate cube_node speech_to_text stt_whisper mesh_nodes)
target_compile_options(eyeballs PRIVATE -Wall -Wextra)
target_link_options(eyeballs PRIVATE -u ANativeActivity_onCreate)
