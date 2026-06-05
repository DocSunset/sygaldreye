add_library(hrtf_spatializer STATIC hrtf_spatializer.cpp)
target_include_directories(hrtf_spatializer PUBLIC .)
target_include_directories(hrtf_spatializer PUBLIC $ENV{EIGEN3_INCLUDE_DIR})
target_link_libraries(hrtf_spatializer PUBLIC biquad_filter)
target_compile_options(hrtf_spatializer PRIVATE -Wall -Wextra)

set(GTEST_ROOT $ENV{ANDROID_NDK_ROOT}/sources/third_party/googletest)
add_library(hrtf_spatializer_gtest STATIC
    ${GTEST_ROOT}/src/gtest-all.cc
    ${GTEST_ROOT}/src/gtest_main.cc)
target_include_directories(hrtf_spatializer_gtest PUBLIC ${GTEST_ROOT}/include PRIVATE ${GTEST_ROOT})

add_executable(hrtf_spatializer_test hrtf_spatializer.test.cpp)
target_link_libraries(hrtf_spatializer_test PRIVATE hrtf_spatializer synth_core hrtf_spatializer_gtest)
target_compile_options(hrtf_spatializer_test PRIVATE -Wall -Wextra)
