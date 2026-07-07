#pragma once

namespace syg::movements {

inline constexpr int hello_cosine_rate = 48000;
inline constexpr int hello_cosine_block = 128;

// Tick the hand-frozen hello-cosine movement for `frames` samples, calling
// sink(block, n) once per block. Headless: the sink is the transducer.
void render_hello_cosine(int frames, void (*sink)(void* ctx, const float* block, int n),
                         void* ctx);

}  // namespace syg::movements
