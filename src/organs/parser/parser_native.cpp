// clause: machinery — the parser organ as a linked native (presence is a
// tape choice; the machinery itself lives beside this TU)
#include "crown.hpp"

namespace syg::organs {

extern const syg::crown::native_type parser_native;
const syg::crown::native_type parser_native{
    "parser", [] { return static_cast<void*>(nullptr); }, [](void*) {},
    [](void*, const char*, double) {}, [](void*, const char*, const char*) {},
    [](void*, const float* const*, float* const*, int) noexcept {}, {}, {}};

}  // namespace syg::organs
