add_library(ui_nodes STATIC ui_nodes.cpp)
target_include_directories(ui_nodes PUBLIC . $ENV{EIGEN3_INCLUDE_DIR})
target_link_libraries(ui_nodes PUBLIC vr_panel rgba_shader sygaldry_endpoints)
target_compile_options(ui_nodes PRIVATE -Wall -Wextra)
