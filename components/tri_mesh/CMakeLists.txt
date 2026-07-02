add_library(tri_mesh INTERFACE)
target_include_directories(tri_mesh INTERFACE . $ENV{EIGEN3_INCLUDE_DIR})

if(NOT CMAKE_CROSSCOMPILING)
    find_package(GTest REQUIRED)
    add_executable(tri_mesh_test tri_mesh.test.cpp)
    target_link_libraries(tri_mesh_test PRIVATE tri_mesh GTest::gtest_main)
    target_compile_options(tri_mesh_test PRIVATE -Wall -Wextra)
endif()
