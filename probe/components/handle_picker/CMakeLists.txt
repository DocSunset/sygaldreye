add_library(handle_picker STATIC handle_picker.cpp)
target_include_directories(handle_picker PUBLIC . $ENV{EIGEN3_INCLUDE_DIR})
target_link_libraries(handle_picker PUBLIC editor_layout signal_graph sygaldry_endpoints)
target_compile_options(handle_picker PRIVATE -Wall -Wextra)

set(GTEST_ROOT $ENV{ANDROID_NDK_ROOT}/sources/third_party/googletest)
if(EXISTS ${GTEST_ROOT})
    add_library(handle_picker_gtest STATIC
        ${GTEST_ROOT}/src/gtest-all.cc
        ${GTEST_ROOT}/src/gtest_main.cc)
    target_include_directories(handle_picker_gtest PUBLIC ${GTEST_ROOT}/include PRIVATE ${GTEST_ROOT})

    add_executable(handle_picker_test handle_picker.test.cpp)
    target_link_libraries(handle_picker_test PRIVATE handle_picker math_nodes handle_picker_gtest)
    target_compile_options(handle_picker_test PRIVATE -Wall -Wextra)
endif()
