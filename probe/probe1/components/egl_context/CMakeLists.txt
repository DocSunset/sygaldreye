add_library(egl_context STATIC egl_context.cpp)
target_include_directories(egl_context PUBLIC .)
target_link_libraries(egl_context PUBLIC EGL openxr_loader PRIVATE log)
target_compile_options(egl_context PRIVATE -Wall -Wextra)

set(GTEST_ROOT $ENV{ANDROID_NDK_ROOT}/sources/third_party/googletest)
add_library(egl_context_gtest STATIC
    ${GTEST_ROOT}/src/gtest-all.cc
    ${GTEST_ROOT}/src/gtest_main.cc)
target_include_directories(egl_context_gtest PUBLIC ${GTEST_ROOT}/include PRIVATE ${GTEST_ROOT})
add_executable(egl_context_test egl_context.test.cpp)
target_link_libraries(egl_context_test PRIVATE egl_context egl_context_gtest GLESv3)
target_compile_options(egl_context_test PRIVATE -Wall -Wextra)
