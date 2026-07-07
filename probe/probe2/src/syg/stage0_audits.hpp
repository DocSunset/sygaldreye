#pragma once
#include <string>

namespace syg::harness {

int boot_audit(const std::string& empty_objdir);  // SZ-7 / SZ-8
int unfreeze_stage0();                            // SZ-4
int park_audit();                                 // SZ-5

}  // namespace syg::harness
