add_library(chime_synth STATIC chime_synth.cpp)
target_include_directories(chime_synth PUBLIC .)
target_link_libraries(chime_synth PUBLIC synth_core audio_scene)
target_compile_options(chime_synth PRIVATE -Wall -Wextra)

set(GTEST_ROOT $ENV{ANDROID_NDK_ROOT}/sources/third_party/googletest)
add_library(chime_synth_gtest STATIC
    ${GTEST_ROOT}/src/gtest-all.cc
    ${GTEST_ROOT}/src/gtest_main.cc)
target_include_directories(chime_synth_gtest PUBLIC ${GTEST_ROOT}/include PRIVATE ${GTEST_ROOT})

add_executable(chime_synth_test chime_synth.test.cpp)
target_link_libraries(chime_synth_test PRIVATE chime_synth chime_synth_gtest)
target_compile_options(chime_synth_test PRIVATE -Wall -Wextra)
