#include "stage0.hpp"

#include <algorithm>
#include <stdexcept>

#include "boot_tape.hpp"
#include "registry_face/registry_face.hpp"

namespace syg::stage0 {

const char* tape() { return boot_tape; }

peer::peer() {
  plan_ = std::make_unique<crown::plan>(organs::registered_natives(), 128);
  auto ops = crown::read_tape(boot_tape);
  boot_ops_ = static_cast<int>(ops.size());
  for (auto& o : ops) plan_->submit(std::move(o));
  plan_->tick(0);  // the boot boundary: pure op application (SZ-8)
  for (const auto& r : plan_->nodes()) parked_.push_back(r.id);
}

void peer::submit(crown::op o) {
  auto names_parked = [&](const std::string& route) {
    auto id = route.substr(0, route.find('/'));
    return std::find(parked_.begin(), parked_.end(), id) != parked_.end();
  };
  if ((o.what == crown::op::kind::remove && names_parked(o.a)) ||
      (o.what == crown::op::kind::write_default && names_parked(o.a)) ||
      (o.what == crown::op::kind::instantiate && names_parked(o.a)) ||
      ((o.what == crown::op::kind::link || o.what == crown::op::kind::unlink) &&
       (names_parked(o.a) || names_parked(o.b))))
    throw std::runtime_error(
        "stage 0 rejects runtime edits: '" + o.a +
        "' is part of the parked boot graph (SZ-5.2 — the supervisor that "
        "cannot be edited away)");
  plan_->submit(std::move(o));
}

}  // namespace syg::stage0
