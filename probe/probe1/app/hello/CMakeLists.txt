add_executable(app_hello main.cpp)
target_link_libraries(app_hello PRIVATE hello)
target_compile_options(app_hello PRIVATE -Wall -Wextra -Wpedantic)
