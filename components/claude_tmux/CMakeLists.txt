if(NOT CMAKE_CROSSCOMPILING)
    add_library(claude_tmux STATIC claude_tmux.cpp)
    target_include_directories(claude_tmux PUBLIC .)
    target_link_libraries(claude_tmux PUBLIC sygaldry_endpoints worker)
    target_compile_options(claude_tmux PRIVATE -Wall -Wextra)

    set(GTEST_ROOT $ENV{ANDROID_NDK_ROOT}/sources/third_party/googletest)
    if(EXISTS ${GTEST_ROOT})
        add_library(claude_tmux_gtest STATIC
            ${GTEST_ROOT}/src/gtest-all.cc
            ${GTEST_ROOT}/src/gtest_main.cc)
        target_include_directories(claude_tmux_gtest PUBLIC ${GTEST_ROOT}/include PRIVATE ${GTEST_ROOT})

        add_executable(claude_tmux_test claude_tmux.test.cpp)
        target_link_libraries(claude_tmux_test PRIVATE claude_tmux eyeballs_node_abi claude_tmux_gtest)
        target_compile_options(claude_tmux_test PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endif()
