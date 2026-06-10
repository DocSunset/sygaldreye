add_library(component_registry STATIC component_registry.cpp)
target_include_directories(component_registry PUBLIC . $ENV{EIGEN3_INCLUDE_DIR})
# signal_graph/subgraph_node form a genuine static-lib cycle with this target;
# CMake resolves declared cycles by repeating archives at link time.
target_link_libraries(component_registry PUBLIC eyeballs_node_abi signal_graph subgraph_node PRIVATE dl)
target_compile_options(component_registry PRIVATE -Wall -Wextra)
if(ANDROID)
    target_link_libraries(component_registry PRIVATE log)
endif()

set(GTEST_ROOT $ENV{ANDROID_NDK_ROOT}/sources/third_party/googletest)
add_library(component_registry_gtest STATIC
    ${GTEST_ROOT}/src/gtest-all.cc
    ${GTEST_ROOT}/src/gtest_main.cc)
target_include_directories(component_registry_gtest PUBLIC ${GTEST_ROOT}/include PRIVATE ${GTEST_ROOT})

add_executable(component_registry_test component_registry.test.cpp)
# subgraph_node and signal_graph needed to resolve subgraph/parse_graph symbols.
target_link_libraries(component_registry_test PRIVATE subgraph_node signal_graph component_registry component_registry_gtest)
target_compile_options(component_registry_test PRIVATE -Wall -Wextra -Wpedantic)
