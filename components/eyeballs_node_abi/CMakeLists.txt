add_library(eyeballs_node_abi INTERFACE)
target_include_directories(eyeballs_node_abi INTERFACE .)
target_link_libraries(eyeballs_node_abi INTERFACE param_registry)

set(GTEST_ROOT $ENV{ANDROID_NDK_ROOT}/sources/third_party/googletest)
add_library(eyeballs_node_abi_gtest STATIC
    ${GTEST_ROOT}/src/gtest-all.cc
    ${GTEST_ROOT}/src/gtest_main.cc)
target_include_directories(eyeballs_node_abi_gtest PUBLIC ${GTEST_ROOT}/include PRIVATE ${GTEST_ROOT})

add_executable(eyeballs_node_abi_test eyeballs_node_abi.test.cpp)
target_link_libraries(eyeballs_node_abi_test PRIVATE eyeballs_node_abi eyeballs_node_abi_gtest)
target_compile_options(eyeballs_node_abi_test PRIVATE -Wall -Wextra -Wpedantic)
