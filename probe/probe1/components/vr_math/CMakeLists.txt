add_library(vr_math STATIC vr_math.cpp)
target_include_directories(vr_math PUBLIC . $ENV{EIGEN3_INCLUDE_DIR} $ENV{OPENXR_SDK_SRC}/include)
target_link_libraries(vr_math PRIVATE openxr_loader)
target_compile_options(vr_math PRIVATE -Wall -Wextra -Wpedantic)

set(GTEST_ROOT $ENV{ANDROID_NDK_ROOT}/sources/third_party/googletest)
add_library(vr_math_gtest STATIC
    ${GTEST_ROOT}/src/gtest-all.cc
    ${GTEST_ROOT}/src/gtest_main.cc)
target_include_directories(vr_math_gtest PUBLIC ${GTEST_ROOT}/include PRIVATE ${GTEST_ROOT})

add_executable(vr_math_test vr_math.test.cpp)
target_link_libraries(vr_math_test PRIVATE vr_math vr_math_gtest)
target_compile_options(vr_math_test PRIVATE -Wall -Wextra -Wpedantic)
