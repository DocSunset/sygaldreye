set(NDK_GLUE $ENV{ANDROID_NDK_ROOT}/sources/android/native_app_glue)

add_library(native_app_glue STATIC ${NDK_GLUE}/android_native_app_glue.c)
target_include_directories(native_app_glue PUBLIC ${NDK_GLUE})

add_library(eyeballs SHARED app.cpp xr.cpp)
target_link_libraries(eyeballs PRIVATE native_app_glue android log openxr_loader renderer xr_session frame_loop scene input vr_math cube_mesh text_mesh water_surface sky_dome http_server mdns_advertiser param_registry mic_capture push_to_talk eyeballs_node_abi component_registry signal_graph vr_editor vr_panel ray_selector rd_gpu xr_sources renderer_node atmos_synth rain_synth fire_synth engine_synth water_synth creature_synth chime_synth lissajous aurora chladni terrain_generator particle_system reaction_diffusion sun_light trigger_edge ptt_gate)
target_compile_options(eyeballs PRIVATE -Wall -Wextra)
target_link_options(eyeballs PRIVATE -u ANativeActivity_onCreate)
