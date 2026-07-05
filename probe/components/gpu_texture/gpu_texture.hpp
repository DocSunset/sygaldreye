// Copyright 2025 Travis West
#pragma once
#include "sygaldry_endpoints.hpp"
#include <string_view>

#ifdef __ANDROID__
#  include <GLES3/gl3.h>
#else
#  include <GLES2/gl2.h>
   // Host GLES2 headers may not define these typedefs
   using GLenum = unsigned int;
   using GLuint = unsigned int;
#endif

// GPU-side texture handle passed between GPU nodes via the signal graph.
struct GpuTexture {
    GLuint       id              = 0;
    int          width           = 0;
    int          height          = 0;
    unsigned int internal_format = 0x881A;  // GL_RGBA16F
    unsigned int filter          = 0x2601;  // GL_LINEAR
    bool valid() const { return id != 0; }
};

// texture_output<Name, InternalFormat> — output port carrying a GpuTexture.
template<fixed_string Name, unsigned int InternalFormat = 0x881A>
struct texture_output {
    static consteval std::string_view name() { return Name; }
    static constexpr unsigned int internal_format = InternalFormat;
    GpuTexture value{};
};
