add_library(water_surface STATIC water_surface.cpp)
target_include_directories(water_surface PUBLIC .)
target_include_directories(water_surface PUBLIC $ENV{EIGEN3_INCLUDE_DIR})
target_link_libraries(water_surface PUBLIC tri_mesh gl_program PRIVATE log_component)
target_compile_options(water_surface PRIVATE -Wall -Wextra)

if(CMAKE_CROSSCOMPILING)
    target_link_libraries(water_surface PRIVATE GLESv3)

    set(GTEST_ROOT $ENV{ANDROID_NDK_ROOT}/sources/third_party/googletest)
    add_library(water_surface_gtest STATIC
        ${GTEST_ROOT}/src/gtest-all.cc
        ${GTEST_ROOT}/src/gtest_main.cc)
    target_include_directories(water_surface_gtest PUBLIC ${GTEST_ROOT}/include PRIVATE ${GTEST_ROOT})

    add_executable(water_surface_test water_surface.test.cpp)
    target_link_libraries(water_surface_test PRIVATE water_surface egl_context water_surface_gtest GLESv3)
    target_compile_options(water_surface_test PRIVATE -Wall -Wextra)
else()
    target_link_libraries(water_surface PRIVATE GLESv2)
endif()
