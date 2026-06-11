add_library(signal_graph STATIC
    signal_graph_serial.cpp
    signal_graph_migrate.cpp
    signal_graph_sort.cpp
    signal_graph_plan.cpp
    signal_graph_tick.cpp)
target_include_directories(signal_graph PUBLIC . ../subgraph_node ../component_registry $ENV{EIGEN3_INCLUDE_DIR})
target_link_libraries(signal_graph PUBLIC component_registry eyeballs_node_abi gpu_texture sygaldry_endpoints port_schema_reader port_types)
target_compile_options(signal_graph PRIVATE -Wall -Wextra)

set(GTEST_ROOT $ENV{ANDROID_NDK_ROOT}/sources/third_party/googletest)
if(EXISTS ${GTEST_ROOT})
    add_library(signal_graph_gtest STATIC
        ${GTEST_ROOT}/src/gtest-all.cc
        ${GTEST_ROOT}/src/gtest_main.cc)
    target_include_directories(signal_graph_gtest PUBLIC ${GTEST_ROOT}/include PRIVATE ${GTEST_ROOT})

    add_executable(signal_graph_test signal_graph.test.cpp)
    target_link_libraries(signal_graph_test PRIVATE subgraph_node signal_graph signal_graph_gtest)
    target_compile_options(signal_graph_test PRIVATE -Wall -Wextra -Wpedantic)
endif()
