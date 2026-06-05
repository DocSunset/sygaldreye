if(NOT CMAKE_CROSSCOMPILING)

find_package(GTest REQUIRED)

if(DEFINED ENV{STB_INCLUDE_DIR})
    set(STB_INCLUDE_DIR "$ENV{STB_INCLUDE_DIR}")
else()
    find_path(STB_INCLUDE_DIR stb_image_write.h REQUIRED)
endif()

add_library(scene_snapshot STATIC scene_snapshot.cpp)
target_include_directories(scene_snapshot PUBLIC . PRIVATE ${STB_INCLUDE_DIR})
target_link_libraries(scene_snapshot PUBLIC EGL GLESv2)
target_compile_options(scene_snapshot PRIVATE -Wall -Wextra)

add_executable(scene_snapshot_test scene_snapshot.test.cpp)
target_include_directories(scene_snapshot_test PRIVATE ${STB_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/components/host_gl_context)
target_link_libraries(scene_snapshot_test PRIVATE
    scene_snapshot host_gl_context GLESv2 GTest::gtest_main)
target_compile_options(scene_snapshot_test PRIVATE -Wall -Wextra)

endif()
