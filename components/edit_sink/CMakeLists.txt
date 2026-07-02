add_library(edit_sink INTERFACE)
target_include_directories(edit_sink INTERFACE . $ENV{EIGEN3_INCLUDE_DIR})
target_link_libraries(edit_sink INTERFACE sygaldry_endpoints event_queue editor_layout)

set(GTEST_ROOT $ENV{ANDROID_NDK_ROOT}/sources/third_party/googletest)
if(EXISTS ${GTEST_ROOT})
    add_library(edit_sink_gtest STATIC
        ${GTEST_ROOT}/src/gtest-all.cc
        ${GTEST_ROOT}/src/gtest_main.cc)
    target_include_directories(edit_sink_gtest PUBLIC ${GTEST_ROOT}/include PRIVATE ${GTEST_ROOT})

    add_executable(edit_sink_test edit_sink.test.cpp)
    target_link_libraries(edit_sink_test PRIVATE edit_sink signal_graph edit_sink_gtest)
    target_compile_options(edit_sink_test PRIVATE -Wall -Wextra)
endif()
