add_library(engine_synth STATIC engine_synth.cpp)
target_include_directories(engine_synth PUBLIC .)
target_link_libraries(engine_synth PUBLIC synth_core biquad_filter audio_scene)
target_compile_options(engine_synth PRIVATE -Wall -Wextra)

set(GTEST_ROOT $ENV{ANDROID_NDK_ROOT}/sources/third_party/googletest)
add_library(engine_synth_gtest STATIC
    ${GTEST_ROOT}/src/gtest-all.cc
    ${GTEST_ROOT}/src/gtest_main.cc)
target_include_directories(engine_synth_gtest PUBLIC ${GTEST_ROOT}/include PRIVATE ${GTEST_ROOT})

add_executable(engine_synth_test engine_synth.test.cpp)
target_link_libraries(engine_synth_test PRIVATE engine_synth engine_synth_gtest)
target_compile_options(engine_synth_test PRIVATE -Wall -Wextra)
