if(NOT CMAKE_CROSSCOMPILING)
    if(DEFINED ENV{STB_INCLUDE_DIR})
        set(STB_INCLUDE_DIR "$ENV{STB_INCLUDE_DIR}")
    else()
        find_path(STB_INCLUDE_DIR stb_image_write.h REQUIRED)
    endif()
    add_library(host_app STATIC host_app.cpp host_input.cpp)
    target_include_directories(host_app PUBLIC .
        PRIVATE ${STB_INCLUDE_DIR} $ENV{GLFW_INCLUDE_DIR})
    target_link_libraries(host_app
        PUBLIC peer_core fly_camera
        PRIVATE fly_camera_node hand_node editor_node spawner_node claude_tmux tts_node whisper_node math_nodes net_nodes ui_nodes star_field
                water_surface sky_dome sun_light cube_node lissajous aurora_curtain
                chladni terrain_generator particle_system reaction_diffusion rd_gpu rd_renderer render_nodes glsl_effect mesh_nodes
                trigger_edge text_label poke_stick poke_button spectrogram osc_node dac_node ugens eyeballs_node_abi
                -L$ENV{GLFW_LIB_DIR} glfw GLESv2)
    target_compile_options(host_app PRIVATE -Wall -Wextra)
endif()
