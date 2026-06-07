add_library(speech_to_text STATIC speech_to_text.cpp)
target_include_directories(speech_to_text PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(speech_to_text PUBLIC sygaldry_endpoints push_to_talk log)
target_compile_options(speech_to_text PRIVATE -Wall -Wextra)
