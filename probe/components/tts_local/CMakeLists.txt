add_library(tts_local STATIC tts_local.cpp)
target_include_directories(tts_local PUBLIC . $ENV{SHERPA_ONNX_ROOT}/include)
target_link_libraries(tts_local PUBLIC sygaldry_endpoints worker
    $ENV{SHERPA_ONNX_ROOT}/lib/libsherpa-onnx-c-api.so)
target_compile_options(tts_local PRIVATE -Wall -Wextra)

find_package(GTest REQUIRED)
add_executable(tts_local_test tts_local.test.cpp)
target_link_libraries(tts_local_test PRIVATE tts_local stt_whisper eyeballs_node_abi GTest::gtest_main)
target_compile_options(tts_local_test PRIVATE -Wall -Wextra)
