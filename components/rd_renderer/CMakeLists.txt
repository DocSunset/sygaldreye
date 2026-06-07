add_library(rd_renderer STATIC rd_renderer.cpp)
target_include_directories(rd_renderer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} $ENV{EIGEN3_INCLUDE_DIR})
target_link_libraries(rd_renderer PUBLIC gpu_texture sygaldry_endpoints)
target_compile_options(rd_renderer PRIVATE -Wall -Wextra)

if(CMAKE_CROSSCOMPILING)
    target_link_libraries(rd_renderer PRIVATE GLESv3)
else()
    target_link_libraries(rd_renderer PRIVATE GLESv2)
endif()

if(NOT CMAKE_CROSSCOMPILING)
    find_package(GTest REQUIRED)
    add_executable(rd_renderer_test rd_renderer.test.cpp)
    target_include_directories(rd_renderer_test PRIVATE
        ${CMAKE_SOURCE_DIR}/components/host_gl_context
        $ENV{EIGEN3_INCLUDE_DIR})
    target_link_libraries(rd_renderer_test PRIVATE rd_renderer host_gl_context GLESv2 GTest::gtest_main)
    target_compile_options(rd_renderer_test PRIVATE -Wall -Wextra)
endif()
