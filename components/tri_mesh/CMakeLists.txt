add_library(tri_mesh STATIC tri_mesh.cpp)
target_include_directories(tri_mesh PUBLIC .)
target_include_directories(tri_mesh PUBLIC $ENV{EIGEN3_INCLUDE_DIR})
target_link_libraries(tri_mesh PRIVATE log_component)
target_compile_options(tri_mesh PRIVATE -Wall -Wextra)

if(EMSCRIPTEN)
    # GL comes from -sFULL_ES3 at final link.
elseif(CMAKE_CROSSCOMPILING)
    target_link_libraries(tri_mesh PUBLIC GLESv3)

    set(GTEST_ROOT $ENV{ANDROID_NDK_ROOT}/sources/third_party/googletest)
    if(EXISTS ${GTEST_ROOT})
        add_library(tri_mesh_gtest STATIC
            ${GTEST_ROOT}/src/gtest-all.cc
            ${GTEST_ROOT}/src/gtest_main.cc)
        target_include_directories(tri_mesh_gtest PUBLIC ${GTEST_ROOT}/include PRIVATE ${GTEST_ROOT})

        add_executable(tri_mesh_test tri_mesh.test.cpp)
        target_link_libraries(tri_mesh_test PRIVATE tri_mesh egl_context tri_mesh_gtest GLESv3)
        target_compile_options(tri_mesh_test PRIVATE -Wall -Wextra)
    endif()
else()
    target_link_libraries(tri_mesh PUBLIC GLESv2)
endif()
