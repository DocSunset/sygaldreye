// clause: machinery — the supervisor organ (spawn-and-park, SZ-5)
#include <cstring>

#include "crown.hpp"

namespace syg::organs {
namespace {

struct super_state {
  double restart_limit = 1;
};

void super_set_num(void* s, const char* port, double v) {
  if (!std::strcmp(port, "restart-limit"))
    static_cast<super_state*>(s)->restart_limit = v;
}

}  // namespace

extern const syg::crown::native_type supervisor_native;
const syg::crown::native_type supervisor_native{
    "supervisor", [] { return static_cast<void*>(new super_state()); },
    [](void* s) { delete static_cast<super_state*>(s); },
    super_set_num, [](void*, const char*, const char*) {},
    [](void*, const float* const*, float* const*, int) noexcept {}, nullptr, {}, {}};

double supervisor_restart_limit(void* state) {
  return static_cast<super_state*>(state)->restart_limit;
}

}  // namespace syg::organs
