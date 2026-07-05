if(NOT CMAKE_CROSSCOMPILING)
    add_executable(spectator spectator_main.cpp)
    target_include_directories(spectator PRIVATE $ENV{EIGEN3_INCLUDE_DIR})
    target_link_libraries(spectator PRIVATE
        host_app
        host_gl_window
        host_gl_context
        GLESv2)
    target_compile_options(spectator PRIVATE -Wall -Wextra)
endif()
