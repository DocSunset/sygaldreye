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

add_executable(lissajous_snapshot lissajous_snapshot.cpp)
target_include_directories(lissajous_snapshot PRIVATE
    ${CMAKE_SOURCE_DIR}/components/host_gl_context
    $ENV{EIGEN3_INCLUDE_DIR})
target_link_libraries(lissajous_snapshot PRIVATE
    scene_snapshot host_gl_context lissajous GLESv2)
target_compile_options(lissajous_snapshot PRIVATE -Wall -Wextra)

add_executable(chladni_snapshot chladni_snapshot.cpp)
target_include_directories(chladni_snapshot PRIVATE
    ${CMAKE_SOURCE_DIR}/components/host_gl_context
    $ENV{EIGEN3_INCLUDE_DIR})
target_link_libraries(chladni_snapshot PRIVATE
    scene_snapshot host_gl_context chladni GLESv2)
target_compile_options(chladni_snapshot PRIVATE -Wall -Wextra)

endif()
