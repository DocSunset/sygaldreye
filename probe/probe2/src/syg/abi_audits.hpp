#pragma once
#include <string>

namespace syg::harness {

// The ch. 13 contract audits (HARNESS.md): each prints one-line JSON.
int hook_audit();                          // ABI-2.1: full hook table, counters
int create_audit(const std::string& who);  // ABI-2.2: good|bad resource timing
int fault_audit();                         // ABI-3.1: declared fault conversion
int quarantine_audit(bool trap = false);   // ABI-3.2 / TCF-4: death, subprocess
int hang_audit();                          // TCF-4: stale heartbeat -> severance

}  // namespace syg::harness
