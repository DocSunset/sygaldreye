if(NOT CMAKE_CROSSCOMPILING)

if(DEFINED ENV{STB_INCLUDE_DIR})
    set(STB_INCLUDE_DIR "$ENV{STB_INCLUDE_DIR}")
else()
    find_path(STB_INCLUDE_DIR stb_image_write.h REQUIRED)
endif()

add_library(scene_snapshot STATIC scene_snapshot.cpp)
target_include_directories(scene_snapshot PUBLIC . PRIVATE ${STB_INCLUDE_DIR} $ENV{EIGEN3_INCLUDE_DIR})
target_link_libraries(scene_snapshot PUBLIC EGL GLESv2)
target_compile_options(scene_snapshot PRIVATE -Wall -Wextra)

add_executable(aurora_snapshot aurora_snapshot.cpp)
target_include_directories(aurora_snapshot PRIVATE
    ${CMAKE_SOURCE_DIR}/components/host_gl_context
    ${CMAKE_SOURCE_DIR}/components/aurora
    $ENV{EIGEN3_INCLUDE_DIR})
target_link_libraries(aurora_snapshot PRIVATE
    scene_snapshot host_gl_context aurora GLESv2)
target_compile_options(aurora_snapshot PRIVATE -Wall -Wextra)

endif()
