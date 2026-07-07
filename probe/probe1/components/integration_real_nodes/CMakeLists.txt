if(NOT CMAKE_CROSSCOMPILING)
    set(GTEST_ROOT $ENV{ANDROID_NDK_ROOT}/sources/third_party/googletest)
    if(EXISTS ${GTEST_ROOT})
        add_library(integration_real_nodes_gtest STATIC
            ${GTEST_ROOT}/src/gtest-all.cc
            ${GTEST_ROOT}/src/gtest_main.cc)
        target_include_directories(integration_real_nodes_gtest
            PUBLIC  ${GTEST_ROOT}/include
            PRIVATE ${GTEST_ROOT})

        add_executable(integration_real_nodes_test integration_real_nodes.test.cpp)
        target_include_directories(integration_real_nodes_test
            PRIVATE $ENV{EIGEN3_INCLUDE_DIR})
        target_link_libraries(integration_real_nodes_test PRIVATE
            integration_real_nodes_gtest
            host_gl_context signal_graph component_registry eyeballs_node_abi
            port_schema_reader water_surface sky_dome lissajous chladni
            reaction_diffusion mesh_nodes tree_generator subgraph_node)
        target_compile_options(integration_real_nodes_test PRIVATE -Wall -Wextra)
    endif()
endif()
