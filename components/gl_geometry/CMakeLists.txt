add_library(gl_geometry STATIC gl_geometry.cpp)
target_include_directories(gl_geometry PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(gl_geometry PUBLIC host_gl_context)
target_compile_options(gl_geometry PRIVATE -Wall -Wextra)

if(CMAKE_CROSSCOMPILING)
    target_link_libraries(gl_geometry PUBLIC GLESv3)
else()
    target_link_libraries(gl_geometry PUBLIC GLESv2)
endif()

if(CMAKE_CROSSCOMPILING)
    set(GTEST_ROOT $ENV{ANDROID_NDK_ROOT}/sources/third_party/googletest)
    add_library(gl_geometry_gtest STATIC
        ${GTEST_ROOT}/src/gtest-all.cc
        ${GTEST_ROOT}/src/gtest_main.cc)
    target_include_directories(gl_geometry_gtest PUBLIC ${GTEST_ROOT}/include PRIVATE ${GTEST_ROOT})

    add_executable(gl_geometry_test gl_geometry.test.cpp)
    target_link_libraries(gl_geometry_test PRIVATE gl_geometry egl_context gl_geometry_gtest GLESv3)
    target_compile_options(gl_geometry_test PRIVATE -Wall -Wextra)
endif()
