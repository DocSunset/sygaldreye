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

if(NOT CMAKE_CROSSCOMPILING)
    find_package(GTest REQUIRED)
    add_executable(scene_snapshot_test scene_snapshot.test.cpp)
    target_include_directories(scene_snapshot_test PRIVATE ${STB_INCLUDE_DIR}
        ${CMAKE_SOURCE_DIR}/components/host_gl_context $ENV{EIGEN3_INCLUDE_DIR})
    target_link_libraries(scene_snapshot_test PRIVATE
        scene_snapshot host_gl_context ${_gles} GTest::gtest_main)
    target_compile_options(scene_snapshot_test PRIVATE -Wall -Wextra)
endif()
