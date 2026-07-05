add_library(mic_input STATIC mic_input.cpp)
target_include_directories(mic_input PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mic_input PUBLIC sygaldry_endpoints audio_engine)
target_compile_options(mic_input PRIVATE -Wall -Wextra)
