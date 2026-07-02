add_library(tree_generator STATIC tree_generator.cpp tree_generator_impl.cpp)
target_include_directories(tree_generator PUBLIC .)
target_include_directories(tree_generator PUBLIC $ENV{EIGEN3_INCLUDE_DIR})
target_link_libraries(tree_generator PUBLIC tri_mesh noise)
target_compile_options(tree_generator PRIVATE -Wall -Wextra)

if(CMAKE_CROSSCOMPILING)
    set(GTEST_ROOT $ENV{ANDROID_NDK_ROOT}/sources/third_party/googletest)
    add_library(tree_generator_gtest STATIC
        ${GTEST_ROOT}/src/gtest-all.cc
        ${GTEST_ROOT}/src/gtest_main.cc)
    target_include_directories(tree_generator_gtest PUBLIC ${GTEST_ROOT}/include PRIVATE ${GTEST_ROOT})

    add_executable(tree_generator_test tree_generator.test.cpp)
    target_link_libraries(tree_generator_test PRIVATE tree_generator tree_generator_gtest)
    target_compile_options(tree_generator_test PRIVATE -Wall -Wextra)
else()
    find_package(GTest REQUIRED)
    add_executable(tree_generator_test tree_generator.test.cpp)
    target_link_libraries(tree_generator_test PRIVATE tree_generator GTest::gtest_main)
    target_compile_options(tree_generator_test PRIVATE -Wall -Wextra)
endif()
