add_library(audio_scene STATIC audio_scene.cpp)
target_include_directories(audio_scene PUBLIC .)
target_include_directories(audio_scene PUBLIC $ENV{EIGEN3_INCLUDE_DIR})
target_link_libraries(audio_scene PUBLIC hrtf_spatializer)
target_compile_options(audio_scene PRIVATE -Wall -Wextra)

set(GTEST_ROOT $ENV{ANDROID_NDK_ROOT}/sources/third_party/googletest)
add_library(audio_scene_gtest STATIC
    ${GTEST_ROOT}/src/gtest-all.cc
    ${GTEST_ROOT}/src/gtest_main.cc)
target_include_directories(audio_scene_gtest PUBLIC ${GTEST_ROOT}/include PRIVATE ${GTEST_ROOT})

add_executable(audio_scene_test audio_scene.test.cpp)
target_link_libraries(audio_scene_test PRIVATE audio_scene synth_core audio_scene_gtest)
target_compile_options(audio_scene_test PRIVATE -Wall -Wextra)
