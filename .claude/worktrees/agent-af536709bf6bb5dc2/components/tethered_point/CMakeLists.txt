add_library(tethered_point STATIC tethered_point.cpp)
target_include_directories(tethered_point PUBLIC .)
target_include_directories(tethered_point PUBLIC $ENV{EIGEN3_INCLUDE_DIR})
target_compile_options(tethered_point PRIVATE -Wall -Wextra)

set(GTEST_ROOT $ENV{ANDROID_NDK_ROOT}/sources/third_party/googletest)
add_library(tethered_point_gtest STATIC
    ${GTEST_ROOT}/src/gtest-all.cc
    ${GTEST_ROOT}/src/gtest_main.cc)
target_include_directories(tethered_point_gtest PUBLIC ${GTEST_ROOT}/include PRIVATE ${GTEST_ROOT})

add_executable(tethered_point_test tethered_point.test.cpp)
target_link_libraries(tethered_point_test PRIVATE tethered_point tethered_point_gtest)
target_compile_options(tethered_point_test PRIVATE -Wall -Wextra)
