add_library(palette_mesh STATIC palette_mesh.cpp)
target_include_directories(palette_mesh PUBLIC . $ENV{EIGEN3_INCLUDE_DIR})
target_link_libraries(palette_mesh PUBLIC editor_layout palette render_payloads tri_mesh text_mesh sygaldry_endpoints)
target_compile_options(palette_mesh PRIVATE -Wall -Wextra)

set(GTEST_ROOT $ENV{ANDROID_NDK_ROOT}/sources/third_party/googletest)
if(EXISTS ${GTEST_ROOT})
    add_library(palette_mesh_gtest STATIC
        ${GTEST_ROOT}/src/gtest-all.cc
        ${GTEST_ROOT}/src/gtest_main.cc)
    target_include_directories(palette_mesh_gtest PUBLIC ${GTEST_ROOT}/include PRIVATE ${GTEST_ROOT})

    add_executable(palette_mesh_test palette_mesh.test.cpp)
    target_link_libraries(palette_mesh_test PRIVATE palette_mesh palette_mesh_gtest)
    target_compile_options(palette_mesh_test PRIVATE -Wall -Wextra)
endif()
