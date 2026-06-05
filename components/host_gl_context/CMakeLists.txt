if(NOT CMAKE_CROSSCOMPILING)

add_library(host_gl_context STATIC host_gl_context.cpp)
target_include_directories(host_gl_context PUBLIC .)
target_link_libraries(host_gl_context PUBLIC EGL)
target_compile_options(host_gl_context PRIVATE -Wall -Wextra)

endif()
