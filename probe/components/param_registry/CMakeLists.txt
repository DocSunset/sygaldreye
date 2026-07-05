add_library(param_registry INTERFACE)
target_include_directories(param_registry INTERFACE .)
target_link_libraries(param_registry INTERFACE sygaldry_endpoints)

set(GTEST_ROOT $ENV{ANDROID_NDK_ROOT}/sources/third_party/googletest)
if(EXISTS ${GTEST_ROOT})
add_library(param_registry_gtest STATIC
    ${GTEST_ROOT}/src/gtest-all.cc
    ${GTEST_ROOT}/src/gtest_main.cc)
target_include_directories(param_registry_gtest PUBLIC ${GTEST_ROOT}/include PRIVATE ${GTEST_ROOT})

add_executable(param_registry_test param_registry.test.cpp)
target_link_libraries(param_registry_test PRIVATE param_registry param_registry_gtest)
target_compile_options(param_registry_test PRIVATE -Wall -Wextra -Wpedantic)
endif()
