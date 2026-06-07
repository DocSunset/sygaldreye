add_library(cube_node STATIC cube_node.cpp)
target_include_directories(cube_node PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(cube_node PUBLIC cube_mesh material sygaldry_endpoints)
target_compile_options(cube_node PRIVATE -Wall -Wextra -Werror)
