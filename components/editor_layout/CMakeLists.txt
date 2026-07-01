add_library(editor_layout STATIC editor_layout.cpp)
target_include_directories(editor_layout PUBLIC . $ENV{EIGEN3_INCLUDE_DIR})
target_link_libraries(editor_layout PUBLIC signal_graph port_schema_reader sygaldry_endpoints event_queue)
target_compile_options(editor_layout PRIVATE -Wall -Wextra)

set(GTEST_ROOT $ENV{ANDROID_NDK_ROOT}/sources/third_party/googletest)
if(EXISTS ${GTEST_ROOT})
    add_library(editor_layout_gtest STATIC
        ${GTEST_ROOT}/src/gtest-all.cc
        ${GTEST_ROOT}/src/gtest_main.cc)
    target_include_directories(editor_layout_gtest PUBLIC ${GTEST_ROOT}/include PRIVATE ${GTEST_ROOT})

    add_executable(editor_layout_test editor_layout.test.cpp)
    target_link_libraries(editor_layout_test PRIVATE editor_layout math_nodes editor_layout_gtest)
    target_compile_options(editor_layout_test PRIVATE -Wall -Wextra)
endif()
