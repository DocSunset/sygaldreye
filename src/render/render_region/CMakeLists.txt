# The render package's machinery (PKG-4): the GL boundary. Not
# package-gated (SYG_OMIT drops vocabulary, never machinery) — linked
# unconditionally into ./syg, modelled on src/mesh/link. Links the SYSTEM
# EGL/GLES (the sanctioned non-hermetic platform boundary, quarantined to
# this `machinery` component; the core stays hermetic).
add_library(syg_render_region render_region.cpp)
target_include_directories(syg_render_region PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_include_directories(syg_render_region PRIVATE
  ${CMAKE_BINARY_DIR}/generated
  ${CMAKE_SOURCE_DIR}/src/crown
  ${CMAKE_SOURCE_DIR}/src/organs
  /usr/include)
add_dependencies(syg_render_region generated)
target_link_libraries(syg_render_region PUBLIC syg_crown)
target_link_libraries(syg_render_region PRIVATE
  /usr/lib/libEGL.so /usr/lib/libGLESv2.so)
# The system GL stack (libglvnd + the Mesa vendor the loader dlopens at
# runtime) lives in /usr/lib; a nix-built binary won't search it by default.
# Bake it onto the rpath as old-style DT_RPATH (--disable-new-dtags): unlike
# DT_RUNPATH it is TRANSITIVE (resolves libGLESv2's own dep on libGLdispatch)
# and is consulted for the runtime dlopen of the Mesa vendor. Nix store dirs
# still precede /usr/lib, so libc/libstdc++ resolve to nix. Host-GL boundary,
# machinery-scoped (the Quest build uses the NDK's GLES, not this path).
target_link_options(syg_render_region PUBLIC
  "-Wl,--disable-new-dtags" "-Wl,-rpath,/usr/lib")
