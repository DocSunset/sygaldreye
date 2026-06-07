add_library(sun_light STATIC sun_light.cpp)
target_include_directories(sun_light PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(sun_light PUBLIC sygaldry_endpoints)
target_compile_options(sun_light PRIVATE -Wall -Wextra)

if(CMAKE_CROSSCOMPILING)
    set(GTEST_ROOT $ENV{ANDROID_NDK_ROOT}/sources/third_party/googletest)
    add_library(sun_light_gtest STATIC
        ${GTEST_ROOT}/src/gtest-all.cc
        ${GTEST_ROOT}/src/gtest_main.cc)
    target_include_directories(sun_light_gtest PUBLIC ${GTEST_ROOT}/include PRIVATE ${GTEST_ROOT})

    add_executable(sun_light_test sun_light.test.cpp)
    target_link_libraries(sun_light_test PRIVATE sun_light sun_light_gtest)
    target_compile_options(sun_light_test PRIVATE -Wall -Wextra)
endif()
