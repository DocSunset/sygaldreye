file(READ ${CMAKE_CURRENT_SOURCE_DIR}/boot.tape BOOT_TAPE)
configure_file(boot_tape.hpp.in ${CMAKE_BINARY_DIR}/generated/boot_tape.hpp @ONLY)

add_library(syg_stage0 stage0.cpp)
target_include_directories(syg_stage0 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_BINARY_DIR}/generated)
target_link_libraries(syg_stage0 PUBLIC syg_crown syg_organs)
