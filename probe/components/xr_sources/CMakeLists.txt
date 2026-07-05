add_library(xr_sources STATIC xr_sources.cpp)
target_include_directories(xr_sources PUBLIC .)
target_link_libraries(xr_sources PUBLIC sygaldry_endpoints openxr_loader)
target_compile_options(xr_sources PRIVATE -Wall -Wextra)
