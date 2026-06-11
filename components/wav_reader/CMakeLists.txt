add_library(wav_reader STATIC wav_reader.cpp)
target_include_directories(wav_reader PUBLIC .)
target_compile_options(wav_reader PRIVATE -Wall -Wextra)
