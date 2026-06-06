if(DEFINED ENV{STB_INCLUDE_DIR})
    set(STB_INCLUDE_DIR "$ENV{STB_INCLUDE_DIR}")
elseif(NOT CMAKE_CROSSCOMPILING)
    find_path(STB_INCLUDE_DIR stb_image_write.h REQUIRED)
endif()

if(CMAKE_CROSSCOMPILING)
    set(_gles GLESv3)
else()
    set(_gles GLESv2)
endif()

add_library(scene_snapshot STATIC scene_snapshot.cpp)
target_include_directories(scene_snapshot PUBLIC . PRIVATE ${STB_INCLUDE_DIR} $ENV{EIGEN3_INCLUDE_DIR})
target_link_libraries(scene_snapshot PUBLIC EGL ${_gles})
target_compile_options(scene_snapshot PRIVATE -Wall -Wextra)

macro(add_snapshot_exe name)
    add_executable(${name} ${name}.cpp)
    target_include_directories(${name} PRIVATE
        ${CMAKE_SOURCE_DIR}/components/host_gl_context
        $ENV{EIGEN3_INCLUDE_DIR})
    target_link_libraries(${name} PRIVATE scene_snapshot host_gl_context ${_gles} ${ARGN})
    target_compile_options(${name} PRIVATE -Wall -Wextra)
endmacro()

add_snapshot_exe(water_surface_snapshot water_surface)
add_snapshot_exe(sky_dome_snapshot sky_dome sphere_mesh sphere_geometry)
add_snapshot_exe(aurora_snapshot aurora)
add_snapshot_exe(lissajous_snapshot lissajous)
add_snapshot_exe(chladni_snapshot chladni)
add_snapshot_exe(reaction_diffusion_snapshot reaction_diffusion)
add_snapshot_exe(terrain_snapshot terrain_generator sky_dome sphere_mesh sphere_geometry)
add_snapshot_exe(combined_scene_snapshot water_surface sky_dome terrain_generator
    tree_generator lit_shader flat_shader billboard_quad)

if(NOT CMAKE_CROSSCOMPILING)
    find_package(GTest REQUIRED)
    add_executable(scene_snapshot_test scene_snapshot.test.cpp)
    target_include_directories(scene_snapshot_test PRIVATE ${STB_INCLUDE_DIR}
        ${CMAKE_SOURCE_DIR}/components/host_gl_context $ENV{EIGEN3_INCLUDE_DIR})
    target_link_libraries(scene_snapshot_test PRIVATE
        scene_snapshot host_gl_context ${_gles} GTest::gtest_main)
    target_compile_options(scene_snapshot_test PRIVATE -Wall -Wextra)
endif()
