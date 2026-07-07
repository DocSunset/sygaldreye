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

// The hook scope (ADR-034 / CMP-9.1): the executor wraps every node hook;
// compile machinery notes its work, tagged by whether a hook is live.
struct hook_scope {
  bool prev;
  hook_scope() noexcept;
  ~hook_scope() noexcept;
};
bool in_hook() noexcept;
void note_compile_work() noexcept;
// the structural stage's own honest counter (CMP-1.2): expansion or
// region inference actually recomputed (memo hits and identity
// short-circuits don't count)
void note_structural_run() noexcept;
long structural_runs() noexcept;
long compile_work_outside_hooks() noexcept;
void reset_compile_work() noexcept;

}  // namespace syg::abi
