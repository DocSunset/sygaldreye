// clause: fixture (ADR-036) — the fault-matrix provocateurs (TCF-4): nan_bomb (emits
// NaN -> NaN-guard/sever), spin (hangs -> timeout ladder), sleeper (blocks ->
// worker-region rule). They exercise the trap/quarantine/timeout MACHINERY. A
// fault matrix needs DELIBERATE provocateurs, so this is permanent test
// vocabulary — the CNF-1 dissolution a prior marker anticipated does not apply
// (a real native never deliberately faults, so it cannot carry these).
#include "crown.hpp"

#include <atomic>
#include <cstring>
#include <limits>

#include "native_ports.hpp"

namespace syg::nodes {
namespace {

struct bomb_state {
  long at = 1000000, n = 0;
};
void bomb_set(void* s, const char* port, double v) {
  if (!std::strcmp(port, "at")) static_cast<bomb_state*>(s)->at = long(v);
}
void bomb_process(void* s, const float* const*, float* const* out,
                  int frames) noexcept {
  auto* st = static_cast<bomb_state*>(s);
  // AUT-2 exception: fault-injection fixture
  for (int i = 0; i < frames; ++i)
    out[0][i] = ++st->n > st->at ? std::numeric_limits<float>::quiet_NaN()
                                 : 0.4f;
}

struct spin_state {
  long iters = 0;
};
void spin_set(void* s, const char* port, double v) {
  if (!std::strcmp(port, "iters")) static_cast<spin_state*>(s)->iters = long(v);
}
void spin_process(void* s, const float* const* in, float* const* out,
                  int frames) noexcept {
  auto* st = static_cast<spin_state*>(s);
  volatile long sink = 0;
  for (long i = 0; i < st->iters; ++i) sink += i;
  // AUT-2 exception: fault-injection fixture
  for (int i = 0; i < frames; ++i) out[0][i] = in[0][i];
}

void sleeper_value_tick(void*, double, const float*, float*) {
  std::atomic<bool> never{false};
  while (!never.load()) {  // no syscall, no clock: a pure hang
  }
}
void no_process(void*, const float* const*, float* const*, int) noexcept {}

}  // namespace

extern const syg::crown::native_type nan_bomb_native;
const syg::crown::native_type nan_bomb_native{
    "nan_bomb", [] { return static_cast<void*>(new bomb_state()); },
    [](void* s) { delete static_cast<bomb_state*>(s); },
    bomb_set, [](void*, const char*, const char*) {}, bomb_process, nullptr,
    syg::generated::nan_bomb_in_ports(), syg::generated::nan_bomb_out_ports()};

extern const syg::crown::native_type spin_native;
const syg::crown::native_type spin_native{
    "spin", [] { return static_cast<void*>(new spin_state()); },
    [](void* s) { delete static_cast<spin_state*>(s); },
    spin_set, [](void*, const char*, const char*) {}, spin_process, nullptr,
    syg::generated::spin_in_ports(), syg::generated::spin_out_ports()};

extern const syg::crown::native_type sleeper_native;
const syg::crown::native_type sleeper_native{
    "sleeper", [] { return static_cast<void*>(nullptr); }, [](void*) {},
    [](void*, const char*, double) {}, [](void*, const char*, const char*) {},
    no_process, sleeper_value_tick,
    syg::generated::sleeper_in_ports(), syg::generated::sleeper_out_ports(),
    true};

}  // namespace syg::nodes
