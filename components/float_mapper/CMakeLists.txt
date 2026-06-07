add_library(float_mapper STATIC float_mapper.cpp)
target_include_directories(float_mapper PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(float_mapper PUBLIC sygaldry_endpoints)
target_compile_options(float_mapper PRIVATE -Wall -Wextra)

if(CMAKE_CROSSCOMPILING)
    set(GTEST_ROOT $ENV{ANDROID_NDK_ROOT}/sources/third_party/googletest)
    add_library(float_mapper_gtest STATIC
        ${GTEST_ROOT}/src/gtest-all.cc
        ${GTEST_ROOT}/src/gtest_main.cc)
    target_include_directories(float_mapper_gtest PUBLIC ${GTEST_ROOT}/include PRIVATE ${GTEST_ROOT})

    add_executable(float_mapper_test float_mapper.test.cpp)
    target_link_libraries(float_mapper_test PRIVATE float_mapper float_mapper_gtest)
    target_compile_options(float_mapper_test PRIVATE -Wall -Wextra)
endif()
