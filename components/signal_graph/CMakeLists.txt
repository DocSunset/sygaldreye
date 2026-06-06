add_library(signal_graph STATIC signal_graph.cpp)
target_include_directories(signal_graph PUBLIC .)
target_link_libraries(signal_graph PUBLIC component_registry eyeballs_node_abi)
target_compile_options(signal_graph PRIVATE -Wall -Wextra)

set(GTEST_ROOT $ENV{ANDROID_NDK_ROOT}/sources/third_party/googletest)
if(EXISTS ${GTEST_ROOT})
    add_library(signal_graph_gtest STATIC
        ${GTEST_ROOT}/src/gtest-all.cc
        ${GTEST_ROOT}/src/gtest_main.cc)
    target_include_directories(signal_graph_gtest PUBLIC ${GTEST_ROOT}/include PRIVATE ${GTEST_ROOT})

    add_executable(signal_graph_test signal_graph.test.cpp)
    target_link_libraries(signal_graph_test PRIVATE signal_graph signal_graph_gtest)
    target_compile_options(signal_graph_test PRIVATE -Wall -Wextra -Wpedantic)
endif()
