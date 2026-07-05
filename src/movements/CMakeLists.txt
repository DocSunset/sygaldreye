add_library(syg_hello_natives ../nodes/hello_natives/hello_natives.cpp ../nodes/hello_natives/dac_native.cpp)
target_include_directories(syg_hello_natives PUBLIC ../nodes)
target_link_libraries(syg_hello_natives PUBLIC syg_crown syg_movements)

add_library(syg_movements
  hello_cosine/hello_cosine.cpp
  ../nodes/synth_core/synth_core.cpp
  ../nodes/ugens/ugens.cpp)
target_include_directories(syg_movements PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR} ../escapement ../nodes)

add_executable(hello-cosine-movement ../trampoline/host_main.cpp)
target_link_libraries(hello-cosine-movement PRIVATE syg_movements)
set_target_properties(hello-cosine-movement PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
