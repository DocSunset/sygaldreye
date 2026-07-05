add_library(undo_node STATIC undo_node.cpp)
target_include_directories(undo_node PUBLIC . $ENV{EIGEN3_INCLUDE_DIR})
target_link_libraries(undo_node PUBLIC editor_layout signal_graph sygaldry_endpoints)
target_compile_options(undo_node PRIVATE -Wall -Wextra)

set(GTEST_ROOT $ENV{ANDROID_NDK_ROOT}/sources/third_party/googletest)
if(EXISTS ${GTEST_ROOT})
    add_library(undo_node_gtest STATIC
        ${GTEST_ROOT}/src/gtest-all.cc
        ${GTEST_ROOT}/src/gtest_main.cc)
    target_include_directories(undo_node_gtest PUBLIC ${GTEST_ROOT}/include PRIVATE ${GTEST_ROOT})

    add_executable(undo_node_test undo_node.test.cpp)
    target_link_libraries(undo_node_test PRIVATE undo_node math_nodes undo_node_gtest)
    target_compile_options(undo_node_test PRIVATE -Wall -Wextra)
endif()
