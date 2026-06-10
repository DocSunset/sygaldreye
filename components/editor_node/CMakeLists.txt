add_library(editor_node STATIC editor_node.cpp)
target_include_directories(editor_node PUBLIC .)
target_link_libraries(editor_node PUBLIC vr_editor text_mesh sygaldry_endpoints)
target_compile_options(editor_node PRIVATE -Wall -Wextra)
