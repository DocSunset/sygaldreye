if(NOT CMAKE_CROSSCOMPILING)
    if(DEFINED ENV{STB_INCLUDE_DIR})
        set(STB_INCLUDE_DIR "$ENV{STB_INCLUDE_DIR}")
    else()
        find_path(STB_INCLUDE_DIR stb_image_write.h REQUIRED)
    endif()
    add_library(host_app STATIC host_app.cpp host_input.cpp)
    target_include_directories(host_app PUBLIC .
        PRIVATE ${STB_INCLUDE_DIR} $ENV{GLFW_INCLUDE_DIR} ${CMAKE_BINARY_DIR}/generated)
    target_link_libraries(host_app
        PUBLIC peer_core fly_camera
        PRIVATE fly_camera_node hand_node edit_sink graph_source card_mesh card_widgets_mesh card_labels_mesh editor_wires handle_picker wire_drag slider_drag dwell_delete undo_node palette palette_mesh wire_mesh spawner_node claude_tmux tts_node whisper_node math_nodes net_nodes star_field
                water_surface sky_dome sun_light cube_node lissajous aurora_curtain
                chladni terrain_generator particle_system reaction_diffusion mesh_nodes
                trigger_edge text_label spectrogram stt_whisper tts_local osc_node dac_node ugens spatialize_node sample_player eyeballs_node_abi
                render_region color_mesh vertex_color_mesh sprite tree_generator
                poke_stick poke_button ui_nodes rubber_band_controller flat_mesh
                -L$ENV{GLFW_LIB_DIR} glfw GLESv2)
    target_compile_options(host_app PRIVATE -Wall -Wextra)

    find_package(GTest REQUIRED)
    add_executable(host_app_test host_app.test.cpp)
    target_include_directories(host_app_test PRIVATE ${CMAKE_BINARY_DIR}/generated)
    target_compile_definitions(host_app_test
        PRIVATE ASSETS_GRAPHS_DIR="${CMAKE_SOURCE_DIR}/assets/graphs")
    target_link_libraries(host_app_test PRIVATE host_app signal_graph GTest::gtest_main)
    target_compile_options(host_app_test PRIVATE -Wall -Wextra)
endif()
