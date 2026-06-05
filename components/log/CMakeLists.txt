add_library(log_component INTERFACE)
target_include_directories(log_component INTERFACE .)

if(CMAKE_CROSSCOMPILING)
    target_link_libraries(log_component INTERFACE log)
else()
    find_package(spdlog REQUIRED)
    target_link_libraries(log_component INTERFACE spdlog::spdlog)
endif()
