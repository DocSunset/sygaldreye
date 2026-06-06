if(CMAKE_CROSSCOMPILING)

if(DEFINED ENV{STB_INCLUDE_DIR})
    set(STB_INCLUDE_DIR "$ENV{STB_INCLUDE_DIR}")
endif()

add_executable(app_snapshot app_snapshot.cpp)
target_include_directories(app_snapshot PRIVATE
    ${CMAKE_SOURCE_DIR}/components/host_gl_context
    ${CMAKE_SOURCE_DIR}/components/scene_snapshot
    ${STB_INCLUDE_DIR}
    $ENV{EIGEN3_INCLUDE_DIR})
target_link_libraries(app_snapshot PRIVATE
    scene_snapshot host_gl_context
    scene cube_mesh text_mesh water_surface sky_dome
    vr_math GLESv3)
target_compile_options(app_snapshot PRIVATE -Wall -Wextra)

endif()
