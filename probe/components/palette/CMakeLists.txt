add_library(palette STATIC palette.cpp)
target_include_directories(palette PUBLIC . $ENV{EIGEN3_INCLUDE_DIR})
target_link_libraries(palette PUBLIC editor_layout signal_graph sygaldry_endpoints)
target_compile_options(palette PRIVATE -Wall -Wextra)

set(GTEST_ROOT $ENV{ANDROID_NDK_ROOT}/sources/third_party/googletest)
if(EXISTS ${GTEST_ROOT})
    add_library(palette_gtest STATIC
        ${GTEST_ROOT}/src/gtest-all.cc
        ${GTEST_ROOT}/src/gtest_main.cc)
    target_include_directories(palette_gtest PUBLIC ${GTEST_ROOT}/include PRIVATE ${GTEST_ROOT})

    add_executable(palette_test palette.test.cpp)
    target_link_libraries(palette_test PRIVATE palette math_nodes palette_gtest)
    target_compile_options(palette_test PRIVATE -Wall -Wextra)
endif()
