add_library(port_types INTERFACE)
target_include_directories(port_types INTERFACE .)

if(NOT CMAKE_CROSSCOMPILING)
    find_package(GTest REQUIRED)
    add_executable(port_types_test port_types.test.cpp)
    target_link_libraries(port_types_test PRIVATE port_types GTest::gtest_main)
    target_compile_options(port_types_test PRIVATE -Wall -Wextra -Wpedantic)
endif()
