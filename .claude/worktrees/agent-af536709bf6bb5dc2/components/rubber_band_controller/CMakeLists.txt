add_library(rubber_band_controller STATIC rubber_band_controller.cpp)
target_include_directories(rubber_band_controller PUBLIC .)
target_include_directories(rubber_band_controller PUBLIC $ENV{EIGEN3_INCLUDE_DIR})
target_link_libraries(rubber_band_controller PUBLIC
    grab_target
    grab_detector
    sphere_mesh
    cylinder_mesh
    rgba_shader
    text_mesh)
target_compile_options(rubber_band_controller PRIVATE -Wall -Wextra)
