add_library(ws_link STATIC ws_link.cpp)
target_include_directories(ws_link PUBLIC .)
target_link_libraries(ws_link PUBLIC mongoose)
target_compile_options(ws_link PRIVATE -Wall -Wextra)
