cmake_minimum_required(VERSION 3.25)
project(sygaldreye C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra -Wpedantic -Werror)

# The core libraries, resolved to prove the toolchain is whole. Rungs add
# their own sources under src/ (formats first — ch. 18); the syg CLI is the
# single entrypoint that grows a subcommand per rung (conformance/HARNESS.md).
find_package(nlohmann_json REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(SODIUM REQUIRED IMPORTED_TARGET libsodium)
find_package(Threads REQUIRED)

enable_testing()

add_subdirectory(src/syg)
add_subdirectory(src/escapement)

# stage0: reflection-driven node description. Needs GCC 16 -freflection; its
# splices trip -Wpedantic, so it is dropped here (tests are held looser).
add_executable(stage0_test src/stage0.test.cpp)
set_target_properties(stage0_test PROPERTIES CXX_STANDARD 26)
target_compile_options(stage0_test PRIVATE -std=c++26 -freflection -Wno-pedantic)
add_test(NAME stage0 COMMAND stage0_test)

# decree-v1 type constructors over syg_node_t (plain C++20, no reflection).
add_executable(types_test src/types.test.cpp)
add_test(NAME types COMMAND types_test)
add_executable(env_test src/env.test.cpp)
set_target_properties(env_test PROPERTIES CXX_STANDARD 26)   # env.hpp uses #embed (decree bodies)
target_compile_options(env_test PRIVATE -std=c++26 -Wno-pedantic)
add_test(NAME env COMMAND env_test)

# PARKED (pre-registry sketches): src/variant.{hpp,test.cpp} and src/graph.{hpp,test.cpp}
# validated shapes we've since moved past — behavior (place/erase/move/wire) left syg_type
# for the operator registry, which they need to work. They return once it exists. See
# stage0.md. Their targets are intentionally not built.
