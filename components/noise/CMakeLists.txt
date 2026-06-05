add_library(noise STATIC noise.cpp)
target_include_directories(noise PUBLIC .)
target_compile_options(noise PRIVATE -Wall -Wextra)

if(CMAKE_CROSSCOMPILING)
    set(GTEST_ROOT $ENV{ANDROID_NDK_ROOT}/sources/third_party/googletest)
    add_library(noise_gtest STATIC
        ${GTEST_ROOT}/src/gtest-all.cc
        ${GTEST_ROOT}/src/gtest_main.cc)
    target_include_directories(noise_gtest PUBLIC ${GTEST_ROOT}/include PRIVATE ${GTEST_ROOT})

    add_executable(noise_test noise.test.cpp)
    target_link_libraries(noise_test PRIVATE noise noise_gtest)
    target_compile_options(noise_test PRIVATE -Wall -Wextra)
else()
    find_package(GTest REQUIRED)
    add_executable(noise_test noise.test.cpp)
    target_link_libraries(noise_test PRIVATE noise GTest::gtest_main)
    target_compile_options(noise_test PRIVATE -Wall -Wextra)
endif()
