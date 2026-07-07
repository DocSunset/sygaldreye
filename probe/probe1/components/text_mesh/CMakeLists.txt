# Glyph layer (glyph_layout.hpp + atlas/metrics): CPU-only MSDF text layout
# for the render-as-nodes text meshes. The GL TextMesh class is deleted.
add_library(text_mesh STATIC
    text_mesh.cpp
    dejavu_sans_metrics.cpp
    dejavu_sans_png.cpp
)
target_include_directories(text_mesh PUBLIC . $ENV{EIGEN3_INCLUDE_DIR})
target_compile_options(text_mesh PRIVATE -Wall -Wextra)
