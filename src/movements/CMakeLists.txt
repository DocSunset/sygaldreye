add_library(syg_hello_natives
  ../nodes/hello_natives/osc_native.cpp
  ../nodes/hello_natives/lfo_native.cpp
  ../nodes/hello_natives/vca_native.cpp
  ../nodes/hello_natives/dac_native.cpp
  ../nodes/hello_natives/noise_native.cpp
  ../nodes/hello_natives/add_native.cpp
  ../nodes/hello_natives/value_natives.cpp)
target_include_directories(syg_hello_natives PUBLIC ../nodes)
target_include_directories(syg_hello_natives PRIVATE ${CMAKE_BINARY_DIR}/generated ${CMAKE_SOURCE_DIR}/src/authoring)
target_link_libraries(syg_hello_natives PUBLIC syg_crown syg_movements)
add_dependencies(syg_hello_natives generated)

add_library(syg_movements
  hello_cosine/hello_cosine.cpp
  ../nodes/synth_core/synth_core.cpp
  ../nodes/ugens/ugens.cpp)
target_include_directories(syg_movements PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR} ../escapement ../nodes)

add_executable(hello-cosine-movement ../trampoline/host_main.cpp)
target_link_libraries(hello-cosine-movement PRIVATE syg_movements)
set_target_properties(hello-cosine-movement PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
