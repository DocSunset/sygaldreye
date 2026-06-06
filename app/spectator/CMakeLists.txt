if(NOT CMAKE_CROSSCOMPILING)
    add_executable(spectator spectator_main.cpp)
    target_include_directories(spectator PRIVATE $ENV{EIGEN3_INCLUDE_DIR})
    target_link_libraries(spectator PRIVATE
        host_gl_window
        water_surface
        sky_dome)
    target_compile_options(spectator PRIVATE -Wall -Wextra)
endif()
