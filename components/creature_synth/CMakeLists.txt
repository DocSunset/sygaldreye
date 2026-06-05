add_library(creature_synth STATIC creature_synth.cpp)
target_include_directories(creature_synth PUBLIC .)
target_link_libraries(creature_synth PUBLIC synth_core biquad_filter audio_scene)
target_compile_options(creature_synth PRIVATE -Wall -Wextra)

set(GTEST_ROOT $ENV{ANDROID_NDK_ROOT}/sources/third_party/googletest)
add_library(creature_synth_gtest STATIC
    ${GTEST_ROOT}/src/gtest-all.cc
    ${GTEST_ROOT}/src/gtest_main.cc)
target_include_directories(creature_synth_gtest PUBLIC ${GTEST_ROOT}/include PRIVATE ${GTEST_ROOT})

add_executable(creature_synth_test creature_synth.test.cpp)
target_link_libraries(creature_synth_test PRIVATE creature_synth creature_synth_gtest)
target_compile_options(creature_synth_test PRIVATE -Wall -Wextra)
