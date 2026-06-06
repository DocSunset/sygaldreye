add_library(ray_selector STATIC ray_selector.cpp)
target_include_directories(ray_selector PUBLIC .)
target_include_directories(ray_selector PUBLIC $ENV{EIGEN3_INCLUDE_DIR})
target_include_directories(ray_selector PUBLIC $ENV{OPENXR_SDK_SRC}/include)
target_link_libraries(ray_selector PUBLIC vr_panel)
target_compile_options(ray_selector PRIVATE -Wall -Wextra)

set(GTEST_ROOT $ENV{ANDROID_NDK_ROOT}/sources/third_party/googletest)
add_library(ray_selector_gtest STATIC
    ${GTEST_ROOT}/src/gtest-all.cc
    ${GTEST_ROOT}/src/gtest_main.cc)
target_include_directories(ray_selector_gtest PUBLIC ${GTEST_ROOT}/include PRIVATE ${GTEST_ROOT})

add_executable(ray_selector_test ray_selector.test.cpp)
target_include_directories(ray_selector_test PRIVATE
    $ENV{EIGEN3_INCLUDE_DIR}
    $ENV{OPENXR_SDK_SRC}/include
    .)
target_link_libraries(ray_selector_test PRIVATE ray_selector ray_selector_gtest)
if(CMAKE_CROSSCOMPILING)
    target_link_libraries(ray_selector_test PRIVATE GLESv3)
else()
    target_link_libraries(ray_selector_test PRIVATE GLESv2)
endif()
target_compile_options(ray_selector_test PRIVATE -Wall -Wextra)
