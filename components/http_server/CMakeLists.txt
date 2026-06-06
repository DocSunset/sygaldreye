add_library(http_server STATIC http_server.cpp)
target_include_directories(http_server PUBLIC .)
target_link_libraries(http_server PUBLIC mongoose)
target_compile_options(http_server PRIVATE -Wall -Wextra)
