add_library(terrain_generator STATIC terrain_generator.cpp)
target_include_directories(terrain_generator PUBLIC .
    ${CMAKE_SOURCE_DIR}/components/log
    ${CMAKE_SOURCE_DIR}/components/light
    ${CMAKE_SOURCE_DIR}/components/material
    ${CMAKE_SOURCE_DIR}/components/gl_program)
target_include_directories(terrain_generator PUBLIC $ENV{EIGEN3_INCLUDE_DIR})
target_link_libraries(terrain_generator PUBLIC tri_mesh noise gl_program light material log_component sygaldry_endpoints)
target_compile_options(terrain_generator PRIVATE -Wall -Wextra)

if(CMAKE_CROSSCOMPILING)
    set(GTEST_ROOT $ENV{ANDROID_NDK_ROOT}/sources/third_party/googletest)
    add_library(terrain_generator_gtest STATIC
        ${GTEST_ROOT}/src/gtest-all.cc
        ${GTEST_ROOT}/src/gtest_main.cc)
    target_include_directories(terrain_generator_gtest PUBLIC ${GTEST_ROOT}/include PRIVATE ${GTEST_ROOT})

    add_executable(terrain_generator_test terrain_generator.test.cpp)
    target_link_libraries(terrain_generator_test PRIVATE terrain_generator terrain_generator_gtest)
    target_compile_options(terrain_generator_test PRIVATE -Wall -Wextra)
else()
    find_package(GTest REQUIRED)
    add_executable(terrain_generator_test terrain_generator.test.cpp)
    target_link_libraries(terrain_generator_test PRIVATE terrain_generator GTest::gtest_main)
    target_compile_options(terrain_generator_test PRIVATE -Wall -Wextra)
endif()
