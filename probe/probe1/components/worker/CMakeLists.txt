add_library(worker INTERFACE)
target_include_directories(worker INTERFACE .)

if(NOT CMAKE_CROSSCOMPILING)
    find_package(GTest REQUIRED)
    add_executable(worker_test worker.test.cpp)
    target_link_libraries(worker_test PRIVATE worker GTest::gtest_main)
    target_compile_options(worker_test PRIVATE -Wall -Wextra -Wpedantic)
endif()
