add_library(xr_session STATIC xr_session.cpp)
target_include_directories(xr_session PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(xr_session PRIVATE openxr_loader log)
target_compile_options(xr_session PRIVATE -Wall -Wextra)
