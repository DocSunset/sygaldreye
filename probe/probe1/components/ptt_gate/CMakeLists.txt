add_library(ptt_gate STATIC ptt_gate.cpp)
target_include_directories(ptt_gate PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ptt_gate PUBLIC sygaldry_endpoints)
target_compile_options(ptt_gate PRIVATE -Wall -Wextra)

if(CMAKE_CROSSCOMPILING)
    set(GTEST_ROOT $ENV{ANDROID_NDK_ROOT}/sources/third_party/googletest)
    add_library(ptt_gate_gtest STATIC
        ${GTEST_ROOT}/src/gtest-all.cc
        ${GTEST_ROOT}/src/gtest_main.cc)
    target_include_directories(ptt_gate_gtest PUBLIC ${GTEST_ROOT}/include PRIVATE ${GTEST_ROOT})

    add_executable(ptt_gate_test ptt_gate.test.cpp)
    target_link_libraries(ptt_gate_test PRIVATE ptt_gate ptt_gate_gtest)
    target_compile_options(ptt_gate_test PRIVATE -Wall -Wextra)
endif()
