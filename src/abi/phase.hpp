#pragma once

namespace syg::abi {

enum class phase { none = 0, create, prepare, apply, process, destroy };
inline constexpr int phase_count = 6;
inline constexpr const char* phase_names[phase_count] = {
    "none", "create", "prepare", "apply", "process", "destroy"};

phase current() noexcept;

struct phase_guard {
  phase prev;
  explicit phase_guard(phase p) noexcept;
  ~phase_guard() noexcept;
};

struct counters {
  long allocs = 0;
  long locks = 0;
};
counters& counts(phase p) noexcept;  // per-phase, process-lifetime
void reset_counts() noexcept;

void count_alloc() noexcept;    // harness allocator override calls this
void rt_lock_probe() noexcept;  // lock shims call this

// Resource acquisition is prepare's business alone (L9: first-tick lazy,
// fail loud). Anywhere else: the allocation-discipline abort (ABI-2.2).
int acquire_device(const char* name);

}  // namespace syg::abi
