add_library(event_queue INTERFACE)
target_include_directories(event_queue INTERFACE .)

if(NOT CMAKE_CROSSCOMPILING)
    find_package(GTest REQUIRED)
    add_executable(event_queue_test event_queue.test.cpp)
    target_link_libraries(event_queue_test PRIVATE event_queue GTest::gtest_main)
    target_compile_options(event_queue_test PRIVATE -Wall -Wextra -Wpedantic)
endif()
