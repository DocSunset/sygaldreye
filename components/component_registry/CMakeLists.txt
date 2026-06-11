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

if(NOT CMAKE_CROSSCOMPILING)
    # Hot-reload fixtures: the same node compiled twice with different code.
    foreach(v 1 2)
        add_library(hot_test_v${v} MODULE hot_test_node.cpp)
        target_compile_definitions(hot_test_v${v} PRIVATE HOT_TEST_VERSION=${v})
        target_link_libraries(hot_test_v${v} PRIVATE eyeballs_node_abi sygaldry_endpoints param_registry gpu_texture tri_mesh)
        target_include_directories(hot_test_v${v} PRIVATE $ENV{EIGEN3_INCLUDE_DIR})
        # Without this, the descriptor's function-local static gets
        # STB_GNU_UNIQUE binding and glibc unifies it PROCESS-WIDE (even
        # under RTLD_LOCAL) — a reloaded .so would silently reuse the old
        # version's descriptor. Every plugin build needs this flag.
        target_compile_options(hot_test_v${v} PRIVATE -fno-gnu-unique)
    endforeach()
    target_compile_definitions(component_registry_test PRIVATE
        HOT_TEST_V1_PATH="$<TARGET_FILE:hot_test_v1>"
        HOT_TEST_V2_PATH="$<TARGET_FILE:hot_test_v2>")
    add_dependencies(component_registry_test hot_test_v1 hot_test_v2)
endif()
