add_library(graph_source STATIC graph_source.cpp)
target_include_directories(graph_source PUBLIC . $ENV{EIGEN3_INCLUDE_DIR})
target_link_libraries(graph_source PUBLIC signal_graph sygaldry_endpoints)
target_compile_options(graph_source PRIVATE -Wall -Wextra)

set(GTEST_ROOT $ENV{ANDROID_NDK_ROOT}/sources/third_party/googletest)
if(EXISTS ${GTEST_ROOT})
    add_library(graph_source_gtest STATIC
        ${GTEST_ROOT}/src/gtest-all.cc
        ${GTEST_ROOT}/src/gtest_main.cc)
    target_include_directories(graph_source_gtest PUBLIC ${GTEST_ROOT}/include PRIVATE ${GTEST_ROOT})

    add_executable(graph_source_test graph_source.test.cpp)
    target_link_libraries(graph_source_test PRIVATE graph_source signal_graph graph_source_gtest)
    target_compile_options(graph_source_test PRIVATE -Wall -Wextra)
endif()
