add_library(sphere_mesh STATIC sphere_mesh.cpp)
target_include_directories(sphere_mesh PUBLIC .)
target_include_directories(sphere_mesh PUBLIC $ENV{EIGEN3_INCLUDE_DIR})
target_link_libraries(sphere_mesh PUBLIC sphere_geometry log_component)
target_compile_options(sphere_mesh PRIVATE -Wall -Wextra)

if(EMSCRIPTEN)
    # GL comes from -sFULL_ES3 at final link.
elseif(CMAKE_CROSSCOMPILING)
    target_link_libraries(sphere_mesh PUBLIC GLESv3)
else()
    target_link_libraries(sphere_mesh PUBLIC GLESv2)
endif()
