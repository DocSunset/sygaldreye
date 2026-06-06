add_library(sygaldry_endpoints INTERFACE)
target_include_directories(sygaldry_endpoints INTERFACE . $ENV{EIGEN3_INCLUDE_DIR})

set(GTEST_ROOT $ENV{ANDROID_NDK_ROOT}/sources/third_party/googletest)
add_library(sygaldry_endpoints_gtest STATIC
    ${GTEST_ROOT}/src/gtest-all.cc
    ${GTEST_ROOT}/src/gtest_main.cc)
target_include_directories(sygaldry_endpoints_gtest PUBLIC ${GTEST_ROOT}/include PRIVATE ${GTEST_ROOT})

add_executable(sygaldry_endpoints_test sygaldry_endpoints.test.cpp)
target_link_libraries(sygaldry_endpoints_test PRIVATE sygaldry_endpoints sygaldry_endpoints_gtest)
target_compile_options(sygaldry_endpoints_test PRIVATE -Wall -Wextra -Wpedantic)
