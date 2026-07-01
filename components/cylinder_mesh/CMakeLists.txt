add_library(cylinder_mesh STATIC cylinder_mesh.cpp)
target_include_directories(cylinder_mesh PUBLIC . $ENV{EIGEN3_INCLUDE_DIR})
target_link_libraries(cylinder_mesh PUBLIC tri_mesh)
# Eigen's FromTwoVectors trips a -Wmaybe-uninitialized false positive in its
# own headers under host GCC; our code is clean. Match the tree's -Wall -Wextra.
target_compile_options(cylinder_mesh PRIVATE -Wall -Wextra)

if(NOT CMAKE_CROSSCOMPILING)
    find_package(GTest REQUIRED)
    add_executable(cylinder_mesh_test cylinder_mesh.test.cpp)
    target_link_libraries(cylinder_mesh_test PRIVATE cylinder_mesh GTest::gtest_main)
    target_compile_options(cylinder_mesh_test PRIVATE -Wall -Wextra)
endif()
