#pragma once
#include <memory>
#include <string>

#include "crown.hpp"

namespace syg::stage0 {

const char* tape();  // the embedded boot tape

// Boot: replay the embedded tape through the crown over the linked
// registry. The returned plan's boot-built routes are parked: ops that
// target them are refused (SZ-5.2); everything else flows.
class peer {
 public:
  peer();
  crown::plan& plan() { return *plan_; }
  int boot_ops() const { return boot_ops_; }
  // Refuses stage-0 edits with a clear error; forwards the rest.
  void submit(crown::op o);

 private:
  std::unique_ptr<crown::plan> plan_;
  std::vector<std::string> parked_;
  int boot_ops_ = 0;
};

}  // namespace syg::stage0
