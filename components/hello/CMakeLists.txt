add_library(hello hello.cpp)
target_include_directories(hello PUBLIC .)
target_compile_options(hello PRIVATE -Wall -Wextra -Wpedantic)

find_package(Catch2 3 REQUIRED)

add_executable(hello_test hello.test.cpp)
target_link_libraries(hello_test PRIVATE hello Catch2::Catch2WithMain)
target_compile_options(hello_test PRIVATE -Wall -Wextra -Wpedantic)

include(Catch)
catch_discover_tests(hello_test)
