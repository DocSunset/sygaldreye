add_library(vr_panel STATIC vr_panel.cpp)
target_include_directories(vr_panel PUBLIC .)
target_include_directories(vr_panel PUBLIC $ENV{EIGEN3_INCLUDE_DIR})
target_link_libraries(vr_panel PUBLIC rgba_shader)
target_compile_options(vr_panel PRIVATE -Wall -Wextra)

set(GTEST_ROOT $ENV{ANDROID_NDK_ROOT}/sources/third_party/googletest)
add_library(vr_panel_gtest STATIC
    ${GTEST_ROOT}/src/gtest-all.cc
    ${GTEST_ROOT}/src/gtest_main.cc)
target_include_directories(vr_panel_gtest PUBLIC ${GTEST_ROOT}/include PRIVATE ${GTEST_ROOT})

add_executable(vr_panel_test vr_panel.test.cpp)
target_include_directories(vr_panel_test PRIVATE $ENV{EIGEN3_INCLUDE_DIR} .)
target_link_libraries(vr_panel_test PRIVATE vr_panel vr_panel_gtest)
if(CMAKE_CROSSCOMPILING)
    target_link_libraries(vr_panel_test PRIVATE GLESv3)
else()
    target_link_libraries(vr_panel_test PRIVATE GLESv2)
endif()
target_compile_options(vr_panel_test PRIVATE -Wall -Wextra)
