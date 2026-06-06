add_library(rain_synth STATIC rain_synth.cpp)
target_include_directories(rain_synth PUBLIC .)
target_link_libraries(rain_synth PUBLIC synth_core biquad_filter audio_scene sygaldry_endpoints)
target_compile_options(rain_synth PRIVATE -Wall -Wextra)

set(GTEST_ROOT $ENV{ANDROID_NDK_ROOT}/sources/third_party/googletest)
add_library(rain_synth_gtest STATIC
    ${GTEST_ROOT}/src/gtest-all.cc
    ${GTEST_ROOT}/src/gtest_main.cc)
target_include_directories(rain_synth_gtest PUBLIC ${GTEST_ROOT}/include PRIVATE ${GTEST_ROOT})

add_executable(rain_synth_test rain_synth.test.cpp)
target_link_libraries(rain_synth_test PRIVATE rain_synth rain_synth_gtest)
target_compile_options(rain_synth_test PRIVATE -Wall -Wextra)
