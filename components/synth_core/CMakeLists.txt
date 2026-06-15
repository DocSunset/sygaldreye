add_library(synth_core STATIC synth_core.cpp)
target_include_directories(synth_core PUBLIC .)
# block_shell.hpp (generator shell) returns AudioBuffer.
target_link_libraries(synth_core PUBLIC sygaldry_endpoints)
target_compile_options(synth_core PRIVATE -Wall -Wextra)

set(GTEST_ROOT $ENV{ANDROID_NDK_ROOT}/sources/third_party/googletest)

if(EXISTS ${GTEST_ROOT})
add_library(synth_core_gtest STATIC
    ${GTEST_ROOT}/src/gtest-all.cc
    ${GTEST_ROOT}/src/gtest_main.cc)
target_include_directories(synth_core_gtest PUBLIC ${GTEST_ROOT}/include PRIVATE ${GTEST_ROOT})

add_executable(synth_core_test synth_core.test.cpp)
target_link_libraries(synth_core_test PRIVATE synth_core synth_core_gtest)
target_compile_options(synth_core_test PRIVATE -Wall -Wextra)
endif()
