add_library(water_surface STATIC water_surface.cpp)
target_include_directories(water_surface PUBLIC .)
target_include_directories(water_surface PUBLIC $ENV{EIGEN3_INCLUDE_DIR})
target_link_libraries(water_surface PUBLIC tri_mesh render_payloads sygaldry_endpoints)
target_compile_options(water_surface PRIVATE -Wall -Wextra)

if(CMAKE_CROSSCOMPILING)
    set(GTEST_ROOT $ENV{ANDROID_NDK_ROOT}/sources/third_party/googletest)
    if(EXISTS ${GTEST_ROOT})
        add_library(water_surface_gtest STATIC
            ${GTEST_ROOT}/src/gtest-all.cc
            ${GTEST_ROOT}/src/gtest_main.cc)
        target_include_directories(water_surface_gtest PUBLIC ${GTEST_ROOT}/include PRIVATE ${GTEST_ROOT})

        add_executable(water_surface_test water_surface.test.cpp)
        target_link_libraries(water_surface_test PRIVATE water_surface param_registry water_surface_gtest)
        target_compile_options(water_surface_test PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endif()
