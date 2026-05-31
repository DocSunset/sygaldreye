add_library(renderer STATIC renderer.cpp)
target_include_directories(renderer PUBLIC .)
target_link_libraries(renderer PUBLIC EGL GLESv3 log openxr_loader)
target_compile_options(renderer PRIVATE -Wall -Wextra)

set(GTEST_ROOT $ENV{ANDROID_NDK_ROOT}/sources/third_party/googletest)
add_library(renderer_gtest STATIC
    ${GTEST_ROOT}/src/gtest-all.cc
    ${GTEST_ROOT}/src/gtest_main.cc)
target_include_directories(renderer_gtest PUBLIC ${GTEST_ROOT}/include PRIVATE ${GTEST_ROOT})

add_executable(renderer_test renderer.test.cpp)
target_link_libraries(renderer_test PRIVATE renderer renderer_gtest)
target_compile_options(renderer_test PRIVATE -Wall -Wextra)
