add_library(fire_synth STATIC fire_synth.cpp)
target_include_directories(fire_synth PUBLIC .)
target_link_libraries(fire_synth PUBLIC synth_core biquad_filter audio_scene)
target_compile_options(fire_synth PRIVATE -Wall -Wextra)

set(GTEST_ROOT $ENV{ANDROID_NDK_ROOT}/sources/third_party/googletest)
add_library(fire_synth_gtest STATIC
    ${GTEST_ROOT}/src/gtest-all.cc
    ${GTEST_ROOT}/src/gtest_main.cc)
target_include_directories(fire_synth_gtest PUBLIC ${GTEST_ROOT}/include PRIVATE ${GTEST_ROOT})

add_executable(fire_synth_test fire_synth.test.cpp)
target_link_libraries(fire_synth_test PRIVATE fire_synth fire_synth_gtest)
target_compile_options(fire_synth_test PRIVATE -Wall -Wextra)
