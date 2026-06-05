add_library(grab_detector STATIC grab_detector.cpp)
target_include_directories(grab_detector PUBLIC .)
target_include_directories(grab_detector PUBLIC $ENV{EIGEN3_INCLUDE_DIR})
target_link_libraries(grab_detector PUBLIC grab_target)
target_compile_options(grab_detector PRIVATE -Wall -Wextra)

set(GTEST_ROOT $ENV{ANDROID_NDK_ROOT}/sources/third_party/googletest)
add_library(grab_detector_gtest STATIC
    ${GTEST_ROOT}/src/gtest-all.cc
    ${GTEST_ROOT}/src/gtest_main.cc)
target_include_directories(grab_detector_gtest PUBLIC ${GTEST_ROOT}/include PRIVATE ${GTEST_ROOT})

add_executable(grab_detector_test grab_detector.test.cpp)
target_link_libraries(grab_detector_test PRIVATE grab_detector grab_detector_gtest)
target_compile_options(grab_detector_test PRIVATE -Wall -Wextra)
