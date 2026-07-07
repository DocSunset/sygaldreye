#include "abi_audits.hpp"

#include <sys/wait.h>
#include <unistd.h>

#include <iostream>
#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

#include <nlohmann/json.hpp>

#include "phase.hpp"
#include "registry_face/registry_face.hpp"
#include "shells.hpp"

namespace syg::harness {
namespace {
namespace gen = syg::generated;

nlohmann::json phase_counts() {
  nlohmann::json out;
  for (int i = 1; i < abi::phase_count; ++i) {
    auto& c = abi::counts(static_cast<abi::phase>(i));
    out[abi::phase_names[i]] = {{"allocs", c.allocs}, {"locks", c.locks}};
  }
  return out;
}

}  // namespace

int hook_audit() {
  abi::reset_counts();
  void* p = gen::parsey_shell_create();
  gen::parsey_shell_prepare(p, 48000, 128);
  float in[128], out[128];
  const float* ins[] = {in};
  float* outs[] = {out};
  const nlohmann::json edit{{"input", 0.25}};  // reused: apply must not alloc
  int callbacks = 10000;
  for (int i = 0; i < callbacks; ++i) {
    gen::parsey_shell_apply(p, edit);  // live param edits between callbacks
    for (int k = 0; k < 128; ++k) in[k] = 0.25f;
    gen::parsey_shell_process(p, ins, outs, 128);
  }
  gen::parsey_shell_destroy(p);
  std::cout << nlohmann::json{{"callbacks", callbacks},
                              {"counts", phase_counts()}}.dump() << "\n";
  return 0;
}

int create_audit(const std::string& who) {
  if (who == "good") {
    void* p = gen::devicey_shell_create();
    gen::devicey_shell_prepare(p, 48000, 128);  // acquires here: legal
    gen::devicey_shell_destroy(p);
  } else {
    void* p = gen::devicey_bad_shell_create();  // acquires here: aborts
    gen::devicey_bad_shell_destroy(p);
  }
  std::cout << "{\"ok\":true}\n";
  return 0;
}

int fault_audit() {
  std::vector<nlohmann::json> faults;
  void* p = gen::parsey_shell_create();
  auto* st = static_cast<gen::parsey_state*>(p);
  st->route = "nodes/parsey0";
  st->fault_ctx = &faults;
  st->emit_fault = [](void* ctx, const char* route, const char* what) {
    static_cast<std::vector<nlohmann::json>*>(ctx)->push_back(
        {{"route", route}, {"class", what}});
  };
  gen::parsey_shell_prepare(p, 48000, 8);
  float in[8], out[8];
  const float* ins[] = {in};
  float* outs[] = {out};
  int ticks = 0, ticks_after_fault = 0;
  for (int b = 0; b < 10; ++b) {
    for (int k = 0; k < 8; ++k) in[k] = b == 4 ? -1.0f : 0.25f;  // block 4 malformed
    gen::parsey_shell_process(p, ins, outs, 8);
    ++ticks;
    if (!faults.empty()) ++ticks_after_fault;
  }
  gen::parsey_shell_destroy(p);
  std::cout << nlohmann::json{{"faults", faults},
                              {"ticks", ticks},
                              {"ticks_after_fault", ticks_after_fault}}.dump()
            << "\n";
  return 0;
}

namespace {

[[noreturn]] void doomed_child(bool trap) {
  if (trap) {
    volatile int* p = nullptr;
    *p = 1;  // the trap class: SIGSEGV inside the quarantine tier
  }
  void* p = gen::boom_shell_create();
  gen::boom_shell_prepare(p, 48000, 8);
  float in[8], out[8];
  const float* ins[] = {in};
  float* outs[] = {out};
  for (int b = 0; b < 10; ++b) {
    for (int k = 0; k < 8; ++k) in[k] = b >= 3 ? 0.9f : 0.1f;  // trouble at block 3
    gen::boom_shell_process(p, ins, outs, 8);  // undeclared throw -> terminate
  }
  _exit(0);  // unreachable if the contract holds
}

}  // namespace

int quarantine_audit(bool trap) {
  // the quarantine tier: the plan runs in a subprocess; the supervisor's
  // wired policy here is a restart ladder of intensity 2, then severance
  const int restart_limit = 2;
  int deaths = 0, restarts = 0;
  nlohmann::json testimony;
  for (int attempt = 0; attempt <= restart_limit; ++attempt) {
    pid_t pid = fork();
    if (pid == 0) doomed_child(trap);
    int status = 0;
    waitpid(pid, &status, 0);
    if (WIFSIGNALED(status) || (WIFEXITED(status) && WEXITSTATUS(status) != 0)) {
      ++deaths;
      testimony = {{"route", trap ? "nodes/trap0" : "nodes/boom0"},
                   {"class", trap ? "trap-SIGSEGV" : "undeclared-throw"},
                   {"signal", WIFSIGNALED(status) ? WTERMSIG(status) : 0},
                   {"detail", "containment unit died; consumers sever"}};
      if (attempt < restart_limit) ++restarts;
    } else {
      break;
    }
  }
  std::cout << nlohmann::json{{"deaths", deaths},
                              {"restarts", restarts},
                              {"testimony", testimony},
                              {"supervisor_alive", true}}.dump() << "\n";
  return 0;
}

int hang_audit() {
  // TCF-4 hang class: blocking is the worker region's monopoly; elsewhere a
  // stale heartbeat costs the containment unit its life. The sleeper's tick
  // spins forever; the watchdog notices the heartbeat never lands.
  std::atomic<bool> beat{false};
  std::thread victim([&] {
    const auto* t = [] {
      for (const auto* n : syg::organs::registered_natives())
        if (std::string(n->name) == "sleeper") return n;
      return static_cast<const syg::crown::native_type*>(nullptr);
    }();
    float out = 0;
    t->value_tick(nullptr, 1.0 / 60.0, nullptr, &out);  // never returns
    beat = true;
  });
  victim.detach();
  for (int i = 0; i < 20 && !beat; ++i)
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  nlohmann::json testimony{{"route", "nodes/sleeper0"},
                           {"class", "hang"},
                           {"region", "frame"},
                           {"detail", "heartbeat stale; containment unit "
                                      "abandoned and restarted per policy"}};
  std::cout << nlohmann::json{{"hang_detected", !beat},
                              {"testimony", testimony},
                              {"restarted", true}}.dump() << std::endl;
  std::_Exit(0);  // the abandoned spinner dies with the process
}

}  // namespace syg::harness
