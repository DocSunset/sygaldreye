add_library(water_synth STATIC water_synth.cpp)
target_include_directories(water_synth PUBLIC .)
target_link_libraries(water_synth PUBLIC synth_core biquad_filter audio_scene sygaldry_endpoints)
target_compile_options(water_synth PRIVATE -Wall -Wextra)

set(GTEST_ROOT $ENV{ANDROID_NDK_ROOT}/sources/third_party/googletest)
add_library(water_synth_gtest STATIC
    ${GTEST_ROOT}/src/gtest-all.cc
    ${GTEST_ROOT}/src/gtest_main.cc)
target_include_directories(water_synth_gtest PUBLIC ${GTEST_ROOT}/include PRIVATE ${GTEST_ROOT})

add_executable(water_synth_test water_synth.test.cpp)
target_link_libraries(water_synth_test PRIVATE water_synth water_synth_gtest)
target_compile_options(water_synth_test PRIVATE -Wall -Wextra)
