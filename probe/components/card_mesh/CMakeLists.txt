add_library(card_mesh INTERFACE)
target_include_directories(card_mesh INTERFACE . $ENV{EIGEN3_INCLUDE_DIR})
target_link_libraries(card_mesh INTERFACE tri_mesh render_payloads sygaldry_endpoints)

set(GTEST_ROOT $ENV{ANDROID_NDK_ROOT}/sources/third_party/googletest)
if(EXISTS ${GTEST_ROOT})
    add_library(card_mesh_gtest STATIC
        ${GTEST_ROOT}/src/gtest-all.cc
        ${GTEST_ROOT}/src/gtest_main.cc)
    target_include_directories(card_mesh_gtest PUBLIC ${GTEST_ROOT}/include PRIVATE ${GTEST_ROOT})

    add_executable(card_mesh_test card_mesh.test.cpp)
    target_link_libraries(card_mesh_test PRIVATE card_mesh card_mesh_gtest)
    target_compile_options(card_mesh_test PRIVATE -Wall -Wextra)
endif()
