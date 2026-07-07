set(GTEST_ROOT $ENV{ANDROID_NDK_ROOT}/sources/third_party/googletest)
if(EXISTS ${GTEST_ROOT})
    add_library(editor_integration_gtest STATIC
        ${GTEST_ROOT}/src/gtest-all.cc
        ${GTEST_ROOT}/src/gtest_main.cc)
    target_include_directories(editor_integration_gtest PUBLIC ${GTEST_ROOT}/include PRIVATE ${GTEST_ROOT})

    add_executable(editor_integration_test editor_integration.test.cpp)
    target_link_libraries(editor_integration_test PRIVATE
        wire_drag slider_drag dwell_delete palette editor_layout
        component_registry signal_graph math_nodes editor_integration_gtest)
    target_compile_options(editor_integration_test PRIVATE -Wall -Wextra)
endif()
