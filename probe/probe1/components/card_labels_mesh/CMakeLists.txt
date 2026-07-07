add_library(card_labels_mesh STATIC card_labels_mesh.cpp)
target_include_directories(card_labels_mesh PUBLIC . $ENV{EIGEN3_INCLUDE_DIR})
target_link_libraries(card_labels_mesh PUBLIC editor_layout render_payloads tri_mesh text_mesh sygaldry_endpoints)
target_compile_options(card_labels_mesh PRIVATE -Wall -Wextra)

set(GTEST_ROOT $ENV{ANDROID_NDK_ROOT}/sources/third_party/googletest)
if(EXISTS ${GTEST_ROOT})
    add_library(card_labels_mesh_gtest STATIC
        ${GTEST_ROOT}/src/gtest-all.cc
        ${GTEST_ROOT}/src/gtest_main.cc)
    target_include_directories(card_labels_mesh_gtest PUBLIC ${GTEST_ROOT}/include PRIVATE ${GTEST_ROOT})

    add_executable(card_labels_mesh_test card_labels_mesh.test.cpp)
    target_link_libraries(card_labels_mesh_test PRIVATE card_labels_mesh math_nodes card_labels_mesh_gtest)
    target_compile_options(card_labels_mesh_test PRIVATE -Wall -Wextra)
endif()
