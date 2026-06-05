add_library(atmos_synth STATIC atmos_synth.cpp)
target_include_directories(atmos_synth PUBLIC .)
target_link_libraries(atmos_synth PUBLIC synth_core biquad_filter audio_scene)
target_compile_options(atmos_synth PRIVATE -Wall -Wextra)

set(GTEST_ROOT $ENV{ANDROID_NDK_ROOT}/sources/third_party/googletest)
add_library(atmos_synth_gtest STATIC
    ${GTEST_ROOT}/src/gtest-all.cc
    ${GTEST_ROOT}/src/gtest_main.cc)
target_include_directories(atmos_synth_gtest PUBLIC ${GTEST_ROOT}/include PRIVATE ${GTEST_ROOT})

add_executable(atmos_synth_test atmos_synth.test.cpp)
target_link_libraries(atmos_synth_test PRIVATE atmos_synth atmos_synth_gtest)
target_compile_options(atmos_synth_test PRIVATE -Wall -Wextra)
