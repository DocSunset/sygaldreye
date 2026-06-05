add_library(scene_graph STATIC scene_graph.cpp)
target_include_directories(scene_graph PUBLIC .)
target_include_directories(scene_graph PUBLIC $ENV{EIGEN3_INCLUDE_DIR})
target_compile_options(scene_graph PRIVATE -Wall -Wextra)

if(CMAKE_CROSSCOMPILING)
    set(GTEST_ROOT $ENV{ANDROID_NDK_ROOT}/sources/third_party/googletest)
    add_library(scene_graph_gtest STATIC
        ${GTEST_ROOT}/src/gtest-all.cc
        ${GTEST_ROOT}/src/gtest_main.cc)
    target_include_directories(scene_graph_gtest PUBLIC ${GTEST_ROOT}/include PRIVATE ${GTEST_ROOT})

    add_executable(scene_graph_test scene_graph.test.cpp)
    target_link_libraries(scene_graph_test PRIVATE scene_graph scene_graph_gtest)
    target_compile_options(scene_graph_test PRIVATE -Wall -Wextra)
else()
    find_package(GTest REQUIRED)
    add_executable(scene_graph_test scene_graph.test.cpp)
    target_link_libraries(scene_graph_test PRIVATE scene_graph GTest::gtest_main)
    target_compile_options(scene_graph_test PRIVATE -Wall -Wextra)
endif()
