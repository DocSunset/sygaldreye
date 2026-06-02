add_library(scene STATIC scene.cpp)
target_include_directories(scene PUBLIC .)
target_include_directories(scene PUBLIC $ENV{EIGEN3_INCLUDE_DIR})
target_compile_options(scene PRIVATE -Wall -Wextra)

set(GTEST_ROOT $ENV{ANDROID_NDK_ROOT}/sources/third_party/googletest)
add_library(scene_gtest STATIC
    ${GTEST_ROOT}/src/gtest-all.cc
    ${GTEST_ROOT}/src/gtest_main.cc)
target_include_directories(scene_gtest PUBLIC ${GTEST_ROOT}/include PRIVATE ${GTEST_ROOT})

add_executable(scene_test scene.test.cpp)
target_link_libraries(scene_test PRIVATE scene scene_gtest)
target_include_directories(scene_test PRIVATE $ENV{EIGEN3_INCLUDE_DIR})
target_compile_options(scene_test PRIVATE -Wall -Wextra)
