// clause: floor — leaf arithmetic (an AUT-3 generated one-liner once the
// stamp exists)
#include "crown.hpp"

namespace syg::nodes {
namespace {

void add_process(void*, const float* const* in, float* const* out,
                 int frames) noexcept {
  for (int i = 0; i < frames; ++i) out[0][i] = in[0][i] + in[1][i];
}

}  // namespace

extern const syg::crown::native_type add_native;
const syg::crown::native_type add_native{
    "add", [] { return static_cast<void*>(nullptr); }, [](void*) {},
    [](void*, const char*, double) {}, [](void*, const char*, const char*) {},
    add_process, {"a", "b"}, {"out"}};

}  // namespace syg::nodes
