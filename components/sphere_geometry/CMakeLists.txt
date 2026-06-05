add_library(sphere_geometry STATIC sphere_geometry.cpp)
target_include_directories(sphere_geometry PUBLIC . $ENV{EIGEN3_INCLUDE_DIR})
target_compile_options(sphere_geometry PRIVATE -Wall -Wextra)

if(CMAKE_CROSSCOMPILING)
    set(GTEST_ROOT $ENV{ANDROID_NDK_ROOT}/sources/third_party/googletest)
    add_library(sphere_geometry_gtest STATIC
        ${GTEST_ROOT}/src/gtest-all.cc
        ${GTEST_ROOT}/src/gtest_main.cc)
    target_include_directories(sphere_geometry_gtest PUBLIC ${GTEST_ROOT}/include PRIVATE ${GTEST_ROOT})

    add_executable(sphere_geometry_test sphere_geometry.test.cpp)
    target_link_libraries(sphere_geometry_test PRIVATE sphere_geometry sphere_geometry_gtest)
    target_compile_options(sphere_geometry_test PRIVATE -Wall -Wextra)
else()
    find_package(GTest REQUIRED)
    add_executable(sphere_geometry_test sphere_geometry.test.cpp)
    target_link_libraries(sphere_geometry_test PRIVATE sphere_geometry GTest::gtest_main)
    target_compile_options(sphere_geometry_test PRIVATE -Wall -Wextra)
endif()
