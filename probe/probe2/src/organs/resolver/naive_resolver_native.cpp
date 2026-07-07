// clause: machinery — the naive_resolver organ as a linked native (presence is a
// tape choice; the machinery itself lives beside this TU)
#include "crown.hpp"

namespace syg::organs {

extern const syg::crown::native_type naive_resolver_native;
const syg::crown::native_type naive_resolver_native{
    "naive_resolver", [] { return static_cast<void*>(nullptr); }, [](void*) {},
    [](void*, const char*, double) {}, [](void*, const char*, const char*) {},
    [](void*, const float* const*, float* const*, int) noexcept {}, nullptr, {}, {}};

}  // namespace syg::organs
