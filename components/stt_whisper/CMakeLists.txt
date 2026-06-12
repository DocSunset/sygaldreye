# whisper.cpp built from source by WHATEVER toolchain is active
# (host, NDK, emscripten) — one version everywhere.
if(NOT TARGET whisper)
    set(WHISPER_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(WHISPER_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
    set(GGML_OPENMP            OFF CACHE BOOL "" FORCE)
    set(BUILD_SHARED_LIBS      OFF CACHE BOOL "" FORCE)
    add_subdirectory($ENV{WHISPER_CPP_SRC} whisper_cpp_build EXCLUDE_FROM_ALL)
endif()

add_library(stt_whisper STATIC stt_whisper.cpp)
target_include_directories(stt_whisper PUBLIC .)
target_link_libraries(stt_whisper PUBLIC sygaldry_endpoints worker whisper)

if(NOT CMAKE_CROSSCOMPILING)
    find_package(GTest REQUIRED)
    add_executable(stt_whisper_test stt_whisper.test.cpp)
    target_link_libraries(stt_whisper_test PRIVATE stt_whisper eyeballs_node_abi GTest::gtest_main)
    target_compile_options(stt_whisper_test PRIVATE -Wall -Wextra)
endif()
target_compile_options(stt_whisper PRIVATE -Wall -Wextra)
