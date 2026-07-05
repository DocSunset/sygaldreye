add_library(slider_drag STATIC slider_drag.cpp)
target_include_directories(slider_drag PUBLIC . $ENV{EIGEN3_INCLUDE_DIR})
target_link_libraries(slider_drag PUBLIC editor_layout signal_graph sygaldry_endpoints)
target_compile_options(slider_drag PRIVATE -Wall -Wextra)

set(GTEST_ROOT $ENV{ANDROID_NDK_ROOT}/sources/third_party/googletest)
if(EXISTS ${GTEST_ROOT})
    add_library(slider_drag_gtest STATIC
        ${GTEST_ROOT}/src/gtest-all.cc
        ${GTEST_ROOT}/src/gtest_main.cc)
    target_include_directories(slider_drag_gtest PUBLIC ${GTEST_ROOT}/include PRIVATE ${GTEST_ROOT})

    add_executable(slider_drag_test slider_drag.test.cpp)
    target_link_libraries(slider_drag_test PRIVATE slider_drag math_nodes slider_drag_gtest)
    target_compile_options(slider_drag_test PRIVATE -Wall -Wextra)
endif()
