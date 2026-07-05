add_library(eye_swapchain STATIC eye_swapchain.cpp)
target_include_directories(eye_swapchain PUBLIC .)
target_link_libraries(eye_swapchain PUBLIC GLESv3 openxr_loader PRIVATE log)
target_compile_options(eye_swapchain PRIVATE -Wall -Wextra)

set(GTEST_ROOT $ENV{ANDROID_NDK_ROOT}/sources/third_party/googletest)
add_library(eye_swapchain_gtest STATIC
    ${GTEST_ROOT}/src/gtest-all.cc
    ${GTEST_ROOT}/src/gtest_main.cc)
target_include_directories(eye_swapchain_gtest PUBLIC ${GTEST_ROOT}/include PRIVATE ${GTEST_ROOT})
add_executable(eye_swapchain_test eye_swapchain.test.cpp)
target_link_libraries(eye_swapchain_test PRIVATE eye_swapchain eye_swapchain_gtest)
target_compile_options(eye_swapchain_test PRIVATE -Wall -Wextra)
