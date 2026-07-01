add_library(cube_node STATIC cube_node.cpp)
target_include_directories(cube_node PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(cube_node PUBLIC tri_mesh render_payloads sygaldry_endpoints)
target_compile_options(cube_node PRIVATE -Wall -Wextra -Werror)
