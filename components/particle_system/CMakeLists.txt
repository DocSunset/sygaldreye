add_library(particle_system STATIC particle_system.cpp)
target_include_directories(particle_system PUBLIC .)
target_include_directories(particle_system PUBLIC $ENV{EIGEN3_INCLUDE_DIR})
target_link_libraries(particle_system PUBLIC gl_program log_component sygaldry_endpoints)
target_compile_options(particle_system PRIVATE -Wall -Wextra)

if(CMAKE_CROSSCOMPILING)
    target_link_libraries(particle_system PUBLIC GLESv3)

    set(GTEST_ROOT $ENV{ANDROID_NDK_ROOT}/sources/third_party/googletest)
    add_library(particle_system_gtest STATIC
        ${GTEST_ROOT}/src/gtest-all.cc
        ${GTEST_ROOT}/src/gtest_main.cc)
    target_include_directories(particle_system_gtest PUBLIC ${GTEST_ROOT}/include PRIVATE ${GTEST_ROOT})

    add_executable(particle_system_test particle_system.test.cpp)
    target_link_libraries(particle_system_test PRIVATE particle_system particle_system_gtest)
    target_compile_options(particle_system_test PRIVATE -Wall -Wextra)
else()
    target_link_libraries(particle_system PUBLIC GLESv2)

    find_package(GTest REQUIRED)
    add_executable(particle_system_test particle_system.test.cpp)
    target_link_libraries(particle_system_test PRIVATE particle_system GTest::gtest_main)
    target_compile_options(particle_system_test PRIVATE -Wall -Wextra)
endif()
