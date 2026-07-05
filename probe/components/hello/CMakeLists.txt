add_library(hello hello.cpp)
target_include_directories(hello PUBLIC .)
target_compile_options(hello PRIVATE -Wall -Wextra -Wpedantic)

set(GTEST_ROOT $ENV{ANDROID_NDK_ROOT}/sources/third_party/googletest)
add_library(gtest_main STATIC
    ${GTEST_ROOT}/src/gtest-all.cc
    ${GTEST_ROOT}/src/gtest_main.cc)
target_include_directories(gtest_main PUBLIC ${GTEST_ROOT}/include PRIVATE ${GTEST_ROOT})

add_executable(hello_test hello.test.cpp)
target_link_libraries(hello_test PRIVATE hello gtest_main)
target_compile_options(hello_test PRIVATE -Wall -Wextra -Wpedantic)
