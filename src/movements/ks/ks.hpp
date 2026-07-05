#pragma once

namespace syg::movements {

void render_ks(int frames, void (*sink)(void*, const float*, int), void* ctx);

}  // namespace syg::movements
