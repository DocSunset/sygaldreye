# BLAKE3 reference C implementation, portable lane only (maturity import,
# ADR-033): deterministic on every target; the flake provides BLAKE3_C_DIR.
if(NOT DEFINED ENV{BLAKE3_C_DIR})
  message(FATAL_ERROR "BLAKE3_C_DIR not set — enter the dev shell (nix develop)")
endif()
set(BLAKE3_C_DIR $ENV{BLAKE3_C_DIR})

add_library(blake3 STATIC
  ${BLAKE3_C_DIR}/blake3.c
  ${BLAKE3_C_DIR}/blake3_dispatch.c
  ${BLAKE3_C_DIR}/blake3_portable.c)
target_include_directories(blake3 PUBLIC ${BLAKE3_C_DIR})
target_compile_definitions(blake3 PRIVATE
  BLAKE3_NO_SSE2 BLAKE3_NO_SSE41 BLAKE3_NO_AVX2 BLAKE3_NO_AVX512 BLAKE3_USE_NEON=0)
# vendored sources compile as-is; our -Werror wall stays ours
target_compile_options(blake3 PRIVATE -w)

add_library(syg_cid cid.cpp)
target_include_directories(syg_cid PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(syg_cid PUBLIC blake3)
