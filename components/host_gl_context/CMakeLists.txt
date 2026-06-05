if(NOT CMAKE_CROSSCOMPILING)

find_package(GTest REQUIRED)

add_library(host_gl_context STATIC host_gl_context.cpp)
target_include_directories(host_gl_context PUBLIC .)
target_link_libraries(host_gl_context PUBLIC EGL)
target_compile_options(host_gl_context PRIVATE -Wall -Wextra)

add_executable(host_gl_context_test host_gl_context.test.cpp)
target_link_libraries(host_gl_context_test PRIVATE host_gl_context GLESv2 GTest::gtest_main)
target_compile_options(host_gl_context_test PRIVATE -Wall -Wextra)

endif()
