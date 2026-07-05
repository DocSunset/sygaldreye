set(GTEST_ROOT $ENV{ANDROID_NDK_ROOT}/sources/third_party/googletest)
if(EXISTS ${GTEST_ROOT})
    add_library(integration_http_gtest STATIC
        ${GTEST_ROOT}/src/gtest-all.cc
        ${GTEST_ROOT}/src/gtest_main.cc)
    target_include_directories(integration_http_gtest
        PUBLIC  ${GTEST_ROOT}/include
        PRIVATE ${GTEST_ROOT})

    add_executable(integration_http_test integration_http.test.cpp)
    target_link_libraries(integration_http_test PRIVATE
        integration_http_gtest http_server eyeballs_node_abi)
    target_compile_options(integration_http_test PRIVATE -Wall -Wextra)
endif()
