add_library(component_registry STATIC component_registry.cpp)
target_include_directories(component_registry PUBLIC .)
target_link_libraries(component_registry PUBLIC eyeballs_node_abi PRIVATE dl)
target_compile_options(component_registry PRIVATE -Wall -Wextra)
if(ANDROID)
    target_link_libraries(component_registry PRIVATE log)
endif()

set(GTEST_ROOT $ENV{ANDROID_NDK_ROOT}/sources/third_party/googletest)
add_library(component_registry_gtest STATIC
    ${GTEST_ROOT}/src/gtest-all.cc
    ${GTEST_ROOT}/src/gtest_main.cc)
target_include_directories(component_registry_gtest PUBLIC ${GTEST_ROOT}/include PRIVATE ${GTEST_ROOT})

add_executable(component_registry_test component_registry.test.cpp)
target_link_libraries(component_registry_test PRIVATE component_registry component_registry_gtest)
target_compile_options(component_registry_test PRIVATE -Wall -Wextra -Wpedantic)
