add_library(trigger_edge STATIC trigger_edge.cpp)
target_include_directories(trigger_edge PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(trigger_edge PUBLIC sygaldry_endpoints)
target_compile_options(trigger_edge PRIVATE -Wall -Wextra)

if(CMAKE_CROSSCOMPILING)
    set(GTEST_ROOT $ENV{ANDROID_NDK_ROOT}/sources/third_party/googletest)
    add_library(trigger_edge_gtest STATIC
        ${GTEST_ROOT}/src/gtest-all.cc
        ${GTEST_ROOT}/src/gtest_main.cc)
    target_include_directories(trigger_edge_gtest PUBLIC ${GTEST_ROOT}/include PRIVATE ${GTEST_ROOT})

    add_executable(trigger_edge_test trigger_edge.test.cpp)
    target_link_libraries(trigger_edge_test PRIVATE trigger_edge trigger_edge_gtest)
    target_compile_options(trigger_edge_test PRIVATE -Wall -Wextra)
endif()
