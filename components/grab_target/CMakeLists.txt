add_library(grab_target INTERFACE)
target_include_directories(grab_target INTERFACE . $ENV{EIGEN3_INCLUDE_DIR})

if(NOT CMAKE_CROSSCOMPILING)
    find_package(GTest REQUIRED)
    add_executable(grab_target_test grab_target.test.cpp)
    target_link_libraries(grab_target_test PRIVATE grab_target GTest::gtest_main)
    target_compile_options(grab_target_test PRIVATE -Wall -Wextra)
endif()
