set(GTEST_ROOT $ENV{ANDROID_NDK_ROOT}/sources/third_party/googletest)

if(ANDROID)
    add_library(audio_output STATIC audio_output_android.cpp)
    target_link_libraries(audio_output PUBLIC aaudio)
else()
    add_library(audio_output STATIC audio_output_host.cpp)

    add_library(audio_output_gtest STATIC
        ${GTEST_ROOT}/src/gtest-all.cc
        ${GTEST_ROOT}/src/gtest_main.cc)
    target_include_directories(audio_output_gtest PUBLIC ${GTEST_ROOT}/include PRIVATE ${GTEST_ROOT})

    add_executable(audio_output_test audio_output.test.cpp)
    target_link_libraries(audio_output_test PRIVATE audio_output audio_output_gtest)
    target_compile_options(audio_output_test PRIVATE -Wall -Wextra)
endif()

target_include_directories(audio_output PUBLIC .)
target_compile_options(audio_output PRIVATE -Wall -Wextra)
