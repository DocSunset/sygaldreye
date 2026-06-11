# Standalone Emscripten project: the browser peer.
#   nix shell nixpkgs#emscripten -c emcmake cmake -S app/web -B build/web -G Ninja
#   cmake --build build/web --target web_app
cmake_minimum_required(VERSION 3.21)
project(sygaldreye_web LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT EMSCRIPTEN)
    message(FATAL_ERROR "configure with emcmake (Emscripten toolchain)")
endif()

# Component CMakeLists read these from the environment; ninja re-runs
# cmake without the caller's env, so persist them as cache vars and
# re-seed the env on every configure.
set(SYGALDREYE_EIGEN_DIR "" CACHE PATH "Eigen include dir (…/include/eigen3)")
set(SYGALDREYE_BOOST_DIR "" CACHE PATH "Boost include dir")
if(NOT SYGALDREYE_EIGEN_DIR OR NOT SYGALDREYE_BOOST_DIR)
    message(FATAL_ERROR "pass -DSYGALDREYE_EIGEN_DIR=… -DSYGALDREYE_BOOST_DIR=…")
endif()
set(ENV{EIGEN3_INCLUDE_DIR} ${SYGALDREYE_EIGEN_DIR})
set(ENV{BOOST_INCLUDE_DIR} ${SYGALDREYE_BOOST_DIR})
include_directories(${SYGALDREYE_BOOST_DIR} ${SYGALDREYE_EIGEN_DIR})

set(COMPONENTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../components)
foreach(c
    sygaldry_endpoints gpu_texture param_registry tri_mesh log gl_program
    glsl_blinn_phong port_types port_schema_reader event_queue
    eyeballs_node_abi signal_graph subgraph_node component_registry
    ws_link remote_node
    sphere_geometry sphere_mesh cube_geometry light material lit_shader
    cube_mesh cube_node sky_dome star_field water_surface sun_light
    math_nodes hand_node fly_camera fly_camera_node
    osc_node dac_node audio_output audio_region spectrogram render_nodes glsl_effect)
    add_subdirectory(${COMPONENTS_DIR}/${c} ${CMAKE_BINARY_DIR}/components/${c})
endforeach()

add_executable(web_app web_main.cpp)
target_link_libraries(web_app PRIVATE
    component_registry signal_graph subgraph_node remote_node ws_link
    fly_camera fly_camera_node hand_node math_nodes
    cube_node sky_dome star_field water_surface sun_light
    osc_node dac_node audio_region spectrogram render_nodes)
target_link_options(web_app PRIVATE
    -sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2 -sFULL_ES3
    -sALLOW_MEMORY_GROWTH -lwebsocket.js
    -sEXPORTED_RUNTIME_METHODS=ccall,cwrap,UTF8ToString
    -sEXPORTED_FUNCTIONS=_main,_web_set_graph,_web_get_graph,_web_connect_peer)
set_target_properties(web_app PROPERTIES SUFFIX ".html")
target_compile_options(web_app PRIVATE -Wall -Wextra)
