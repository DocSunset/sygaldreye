#include "phase.hpp"

#include <cstdio>
#include <cstdlib>

namespace syg::abi {
namespace {
thread_local phase cur = phase::none;
counters table[phase_count];
}  // namespace

phase current() noexcept { return cur; }

phase_guard::phase_guard(phase p) noexcept : prev(cur) { cur = p; }
phase_guard::~phase_guard() noexcept { cur = prev; }

counters& counts(phase p) noexcept { return table[static_cast<int>(p)]; }

void reset_counts() noexcept {
  for (auto& c : table) c = {};
}

void count_alloc() noexcept { ++counts(cur).allocs; }
void rt_lock_probe() noexcept { ++counts(cur).locks; }

int acquire_device(const char* name) {
  if (cur != phase::prepare) {
    std::fprintf(stderr,
                 "allocation-discipline: %s acquired in %s — resources are "
                 "prepare()'s business alone (first-tick lazy, fail loud; L9)\n",
                 name, phase_names[static_cast<int>(cur)]);
    std::abort();
  }
  return 1;  // the "device handle"
}

}  // namespace syg::abi
