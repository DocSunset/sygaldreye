cmake_minimum_required(VERSION 3.21)
project(android-project LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# OpenXR loader
set(DYNAMIC_LOADER ON CACHE BOOL "" FORCE)
set(BUILD_API_LAYERS OFF CACHE BOOL "" FORCE)
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_CONFORMANCE_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_SDK_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory($ENV{OPENXR_SDK_SRC} openxr_sdk EXCLUDE_FROM_ALL)

add_subdirectory(components/hello)
add_subdirectory(app/hello)
add_subdirectory(components/renderer)
add_subdirectory(components/xr_session)
add_subdirectory(components/app)
add_subdirectory(components/vr_math)
add_subdirectory(components/gl_program)
add_subdirectory(components/scene)
