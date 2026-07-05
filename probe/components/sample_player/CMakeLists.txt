add_library(sample_player STATIC sample_player.cpp)
target_include_directories(sample_player PUBLIC . $ENV{EIGEN3_INCLUDE_DIR})
target_link_libraries(sample_player PUBLIC sygaldry_endpoints wav_reader worker)
target_compile_options(sample_player PRIVATE -Wall -Wextra)
