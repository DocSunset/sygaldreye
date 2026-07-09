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
