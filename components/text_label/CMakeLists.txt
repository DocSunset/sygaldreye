add_library(text_label STATIC text_label.cpp)
target_include_directories(text_label PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(text_label PUBLIC sygaldry_endpoints text_mesh)
target_compile_options(text_label PRIVATE -Wall -Wextra -Werror)
