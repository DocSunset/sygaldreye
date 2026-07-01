add_library(wire_drag STATIC wire_drag.cpp)
target_include_directories(wire_drag PUBLIC . $ENV{EIGEN3_INCLUDE_DIR})
target_link_libraries(wire_drag PUBLIC editor_layout port_types signal_graph sygaldry_endpoints)
target_compile_options(wire_drag PRIVATE -Wall -Wextra)

set(GTEST_ROOT $ENV{ANDROID_NDK_ROOT}/sources/third_party/googletest)
if(EXISTS ${GTEST_ROOT})
    add_library(wire_drag_gtest STATIC
        ${GTEST_ROOT}/src/gtest-all.cc
        ${GTEST_ROOT}/src/gtest_main.cc)
    target_include_directories(wire_drag_gtest PUBLIC ${GTEST_ROOT}/include PRIVATE ${GTEST_ROOT})

    add_executable(wire_drag_test wire_drag.test.cpp)
    target_link_libraries(wire_drag_test PRIVATE wire_drag math_nodes wire_drag_gtest)
    target_compile_options(wire_drag_test PRIVATE -Wall -Wextra)
endif()
