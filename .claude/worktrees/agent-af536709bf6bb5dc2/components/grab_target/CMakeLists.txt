add_library(grab_target INTERFACE)
target_include_directories(grab_target INTERFACE .)
target_include_directories(grab_target INTERFACE $ENV{EIGEN3_INCLUDE_DIR})

set(GTEST_ROOT $ENV{ANDROID_NDK_ROOT}/sources/third_party/googletest)
add_library(grab_target_gtest STATIC
    ${GTEST_ROOT}/src/gtest-all.cc
    ${GTEST_ROOT}/src/gtest_main.cc)
target_include_directories(grab_target_gtest PUBLIC ${GTEST_ROOT}/include PRIVATE ${GTEST_ROOT})

add_executable(grab_target_test grab_target.test.cpp)
target_link_libraries(grab_target_test PRIVATE grab_target grab_target_gtest)
target_compile_options(grab_target_test PRIVATE -Wall -Wextra)
