add_library(rubber_band_controller STATIC rubber_band_controller.cpp)
target_include_directories(rubber_band_controller PUBLIC . $ENV{EIGEN3_INCLUDE_DIR})
target_link_libraries(rubber_band_controller PUBLIC
    grab_target grab_detector cylinder_mesh sphere_geometry
    tri_mesh render_payloads sygaldry_endpoints)
target_compile_options(rubber_band_controller PRIVATE -Wall -Wextra)

if(NOT CMAKE_CROSSCOMPILING)
    find_package(GTest REQUIRED)
    add_executable(rubber_band_controller_test rubber_band_controller.test.cpp)
    target_link_libraries(rubber_band_controller_test PRIVATE rubber_band_controller GTest::gtest_main)
    target_compile_options(rubber_band_controller_test PRIVATE -Wall -Wextra)
endif()
