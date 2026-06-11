add_library(poke_button STATIC poke_button.cpp)
target_include_directories(poke_button PUBLIC . $ENV{EIGEN3_INCLUDE_DIR})
target_link_libraries(poke_button PUBLIC sygaldry_endpoints)
target_compile_options(poke_button PRIVATE -Wall -Wextra)
