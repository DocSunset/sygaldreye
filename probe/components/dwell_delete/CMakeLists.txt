add_library(dwell_delete STATIC dwell_delete.cpp)
target_include_directories(dwell_delete PUBLIC . $ENV{EIGEN3_INCLUDE_DIR})
target_link_libraries(dwell_delete PUBLIC editor_layout signal_graph sygaldry_endpoints)
target_compile_options(dwell_delete PRIVATE -Wall -Wextra)

set(GTEST_ROOT $ENV{ANDROID_NDK_ROOT}/sources/third_party/googletest)
if(EXISTS ${GTEST_ROOT})
    add_library(dwell_delete_gtest STATIC
        ${GTEST_ROOT}/src/gtest-all.cc
        ${GTEST_ROOT}/src/gtest_main.cc)
    target_include_directories(dwell_delete_gtest PUBLIC ${GTEST_ROOT}/include PRIVATE ${GTEST_ROOT})

    add_executable(dwell_delete_test dwell_delete.test.cpp)
    target_link_libraries(dwell_delete_test PRIVATE dwell_delete math_nodes dwell_delete_gtest)
    target_compile_options(dwell_delete_test PRIVATE -Wall -Wextra)
endif()
