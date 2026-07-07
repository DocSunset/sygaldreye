add_library(syg_crown crown.cpp tape.cpp)
target_include_directories(syg_crown PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ../escapement)
