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

namespace {
thread_local bool hook_live = false;
long outside_work = 0;
}  // namespace

hook_scope::hook_scope() noexcept : prev(hook_live) { hook_live = true; }
hook_scope::~hook_scope() noexcept { hook_live = prev; }
bool in_hook() noexcept { return hook_live; }
void note_compile_work() noexcept {
  if (!hook_live) ++outside_work;
}
long compile_work_outside_hooks() noexcept { return outside_work; }

namespace {
long structural_work = 0;
}  // namespace
void note_structural_run() noexcept { ++structural_work; }
long structural_runs() noexcept { return structural_work; }
void reset_compile_work() noexcept { outside_work = 0; }

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
