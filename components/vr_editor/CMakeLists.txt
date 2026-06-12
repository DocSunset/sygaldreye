add_library(vr_editor STATIC
        vr_editor.cpp
        vr_editor_handles.cpp
        vr_editor_draw.cpp
    vr_editor_interactions.cpp)
target_include_directories(vr_editor PUBLIC . $ENV{EIGEN3_INCLUDE_DIR})
target_link_libraries(vr_editor PUBLIC
    vr_panel signal_graph text_mesh port_schema_reader port_types)
# Only openxr type headers are used (XrPosef); no loader needed.
target_include_directories(vr_editor PUBLIC $ENV{OPENXR_SDK_SRC}/include)
target_compile_options(vr_editor PRIVATE -Wall -Wextra)
