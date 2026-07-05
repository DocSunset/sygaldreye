cmake_minimum_required(VERSION 3.25)
project(sygaldreye C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra -Wpedantic -Werror)

find_package(nlohmann_json REQUIRED)

add_subdirectory(src/formats/address)
add_subdirectory(src/formats/chunk)
add_subdirectory(src/formats/cid)
add_subdirectory(src/formats/dagcbor)
add_subdirectory(src/abi)
add_subdirectory(src/crown)
add_subdirectory(src/generator)
add_subdirectory(src/movements)
add_subdirectory(src/naming)
add_subdirectory(src/syg)
