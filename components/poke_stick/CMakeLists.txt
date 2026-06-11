add_library(poke_stick STATIC poke_stick.cpp)
target_include_directories(poke_stick PUBLIC . $ENV{EIGEN3_INCLUDE_DIR})
target_link_libraries(poke_stick PUBLIC sygaldry_endpoints)
target_compile_options(poke_stick PRIVATE -Wall -Wextra)
