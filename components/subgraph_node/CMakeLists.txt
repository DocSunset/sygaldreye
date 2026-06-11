add_library(subgraph_node STATIC subgraph_node.cpp subgraph_descriptor.cpp)
target_include_directories(subgraph_node PUBLIC . $ENV{EIGEN3_INCLUDE_DIR})
target_link_libraries(subgraph_node PUBLIC signal_graph eyeballs_node_abi gpu_texture sygaldry_endpoints port_schema_reader)
target_compile_options(subgraph_node PRIVATE -Wall -Wextra -Wpedantic)

set(GTEST_ROOT $ENV{ANDROID_NDK_ROOT}/sources/third_party/googletest)
if(EXISTS ${GTEST_ROOT})
    add_library(subgraph_node_gtest STATIC
        ${GTEST_ROOT}/src/gtest-all.cc
        ${GTEST_ROOT}/src/gtest_main.cc)
    target_include_directories(subgraph_node_gtest PUBLIC ${GTEST_ROOT}/include PRIVATE ${GTEST_ROOT})

    add_executable(subgraph_node_test subgraph_node.test.cpp)
    target_link_libraries(subgraph_node_test PRIVATE subgraph_node signal_graph subgraph_node_gtest)
    target_compile_options(subgraph_node_test PRIVATE -Wall -Wextra -Wpedantic)
endif()
