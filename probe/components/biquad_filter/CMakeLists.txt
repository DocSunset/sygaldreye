add_library(biquad_filter STATIC biquad_filter.cpp)
target_include_directories(biquad_filter PUBLIC .)
target_compile_options(biquad_filter PRIVATE -Wall -Wextra)

set(GTEST_ROOT $ENV{ANDROID_NDK_ROOT}/sources/third_party/googletest)

if(EXISTS ${GTEST_ROOT})
add_library(biquad_filter_gtest STATIC
    ${GTEST_ROOT}/src/gtest-all.cc
    ${GTEST_ROOT}/src/gtest_main.cc)
target_include_directories(biquad_filter_gtest PUBLIC ${GTEST_ROOT}/include PRIVATE ${GTEST_ROOT})

add_executable(biquad_filter_test biquad_filter.test.cpp)
target_link_libraries(biquad_filter_test PRIVATE biquad_filter biquad_filter_gtest)
target_compile_options(biquad_filter_test PRIVATE -Wall -Wextra)
endif()
