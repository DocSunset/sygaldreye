cmake_minimum_required(VERSION 3.21)
project(android-project LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_subdirectory(components/hello)
add_subdirectory(app/hello)
