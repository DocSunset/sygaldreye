add_library(syg_movements
  hello_cosine/hello_cosine.cpp
  ../nodes/synth_core/synth_core.cpp
  ../nodes/ugens/ugens.cpp)
target_include_directories(syg_movements PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR} ../escapement ../nodes)
