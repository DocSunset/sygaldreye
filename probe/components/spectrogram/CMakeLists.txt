add_library(spectrogram STATIC spectrogram.cpp)
target_include_directories(spectrogram PUBLIC . $ENV{EIGEN3_INCLUDE_DIR})
target_link_libraries(spectrogram PUBLIC sygaldry_endpoints gpu_texture)
target_compile_options(spectrogram PRIVATE -Wall -Wextra)
