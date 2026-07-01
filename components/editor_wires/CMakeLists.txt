add_library(editor_wires STATIC editor_wires.cpp)
target_include_directories(editor_wires PUBLIC . $ENV{EIGEN3_INCLUDE_DIR})
target_link_libraries(editor_wires PUBLIC editor_layout signal_graph sygaldry_endpoints)
target_compile_options(editor_wires PRIVATE -Wall -Wextra)

set(GTEST_ROOT $ENV{ANDROID_NDK_ROOT}/sources/third_party/googletest)
if(EXISTS ${GTEST_ROOT})
    add_library(editor_wires_gtest STATIC
        ${GTEST_ROOT}/src/gtest-all.cc
        ${GTEST_ROOT}/src/gtest_main.cc)
    target_include_directories(editor_wires_gtest PUBLIC ${GTEST_ROOT}/include PRIVATE ${GTEST_ROOT})

    add_executable(editor_wires_test editor_wires.test.cpp)
    target_link_libraries(editor_wires_test PRIVATE editor_wires math_nodes editor_wires_gtest)
    target_compile_options(editor_wires_test PRIVATE -Wall -Wextra)
endif()
