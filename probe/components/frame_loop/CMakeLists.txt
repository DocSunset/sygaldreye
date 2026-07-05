add_library(frame_loop STATIC frame_loop.cpp)
target_include_directories(frame_loop PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(frame_loop PRIVATE openxr_loader log)
target_compile_options(frame_loop PRIVATE -Wall -Wextra)
