add_library(star_field STATIC star_field.cpp)
target_include_directories(star_field PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(star_field PUBLIC gl_program sygaldry_endpoints)
target_include_directories(star_field PUBLIC $ENV{EIGEN3_INCLUDE_DIR})
target_link_libraries(star_field PRIVATE log_component)
target_compile_options(star_field PRIVATE -Wall -Wextra)

if(EMSCRIPTEN)
    # GL comes from -sFULL_ES3 at final link.
elseif(CMAKE_CROSSCOMPILING)
    target_link_libraries(star_field PRIVATE GLESv3)
else()
    target_link_libraries(star_field PRIVATE GLESv2)
endif()
