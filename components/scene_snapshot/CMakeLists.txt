if(NOT CMAKE_CROSSCOMPILING)

find_package(GTest REQUIRED)

if(DEFINED ENV{STB_INCLUDE_DIR})
    set(STB_INCLUDE_DIR "$ENV{STB_INCLUDE_DIR}")
else()
    find_path(STB_INCLUDE_DIR stb_image_write.h REQUIRED)
endif()

add_library(scene_snapshot STATIC scene_snapshot.cpp)
target_include_directories(scene_snapshot PUBLIC . PRIVATE ${STB_INCLUDE_DIR} $ENV{EIGEN3_INCLUDE_DIR})
target_link_libraries(scene_snapshot PUBLIC EGL GLESv2)
target_compile_options(scene_snapshot PRIVATE -Wall -Wextra)

add_executable(scene_snapshot_test scene_snapshot.test.cpp)
target_include_directories(scene_snapshot_test PRIVATE ${STB_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/components/host_gl_context $ENV{EIGEN3_INCLUDE_DIR})
target_link_libraries(scene_snapshot_test PRIVATE
    scene_snapshot host_gl_context GLESv2 GTest::gtest_main)
target_compile_options(scene_snapshot_test PRIVATE -Wall -Wextra)

add_executable(water_surface_snapshot water_surface_snapshot.cpp)
target_include_directories(water_surface_snapshot PRIVATE
    ${CMAKE_SOURCE_DIR}/components/host_gl_context
    $ENV{EIGEN3_INCLUDE_DIR})
target_link_libraries(water_surface_snapshot PRIVATE
    scene_snapshot host_gl_context water_surface GLESv2)
target_compile_options(water_surface_snapshot PRIVATE -Wall -Wextra)

add_executable(terrain_snapshot terrain_snapshot.cpp)
target_include_directories(terrain_snapshot PRIVATE
    ${CMAKE_SOURCE_DIR}/components/host_gl_context
    $ENV{EIGEN3_INCLUDE_DIR})
target_link_libraries(terrain_snapshot PRIVATE
    scene_snapshot host_gl_context terrain_generator sky_dome sphere_mesh
    sphere_geometry GLESv2)
target_compile_options(terrain_snapshot PRIVATE -Wall -Wextra)

add_executable(sky_dome_snapshot sky_dome_snapshot.cpp)
target_include_directories(sky_dome_snapshot PRIVATE
    ${CMAKE_SOURCE_DIR}/components/host_gl_context
    $ENV{EIGEN3_INCLUDE_DIR})
target_link_libraries(sky_dome_snapshot PRIVATE
    scene_snapshot host_gl_context sky_dome sphere_mesh sphere_geometry GLESv2)
target_compile_options(sky_dome_snapshot PRIVATE -Wall -Wextra)

add_executable(aurora_snapshot aurora_snapshot.cpp)
target_include_directories(aurora_snapshot PRIVATE
    ${CMAKE_SOURCE_DIR}/components/host_gl_context
    ${CMAKE_SOURCE_DIR}/components/aurora
    $ENV{EIGEN3_INCLUDE_DIR})
target_link_libraries(aurora_snapshot PRIVATE
    scene_snapshot host_gl_context aurora GLESv2)
target_compile_options(aurora_snapshot PRIVATE -Wall -Wextra)

add_executable(combined_scene_snapshot combined_scene_snapshot.cpp)
target_include_directories(combined_scene_snapshot PRIVATE
    ${CMAKE_SOURCE_DIR}/components/host_gl_context
    $ENV{EIGEN3_INCLUDE_DIR})
target_link_libraries(combined_scene_snapshot PRIVATE
    scene_snapshot host_gl_context water_surface sky_dome terrain_generator
    tree_generator lit_shader flat_shader billboard_quad GLESv2)
target_compile_options(combined_scene_snapshot PRIVATE -Wall -Wextra)

endif()
