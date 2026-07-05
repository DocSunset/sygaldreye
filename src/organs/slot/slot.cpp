// clause: machinery — the slot organ
#include "slot/slot.hpp"

#include <map>
#include <string>

namespace syg::organs {

void slot_swap(crown::plan& p, const std::vector<crown::op>& build) {
  using crown::op;
  // stash live state by route (the plan forgets it, so remove won't destroy)
  std::map<std::string, std::pair<const crown::native_type*, void*>> keep;
  for (const auto& r : p.nodes())
    keep[r.id] = {p.type_of(r.id), p.exchange_state(r.id, nullptr)};
  // transactional rebuild: everything out, new structure in, one boundary
  for (const auto& r : std::vector(p.nodes()))
    p.submit({op::kind::remove, r.id, "", ""});
  for (const auto& o : build)
    if (o.what != op::kind::write_default) p.submit(o);
  p.tick(0);
  // migrate: same route, same type -> the old state comes home
  for (const auto& r : p.nodes()) {
    auto it = keep.find(r.id);
    if (it != keep.end() && it->second.first == p.type_of(r.id)) {
      void* fresh = p.exchange_state(r.id, it->second.second);
      p.type_of(r.id)->destroy(fresh);
      it->second.second = nullptr;
    }
  }
  for (auto& [id, tp] : keep)
    if (tp.second) tp.first->destroy(tp.second);  // orphans, never leaked
  // defaults re-apply onto migrated state, at their own boundary
  for (const auto& o : build)
    if (o.what == op::kind::write_default) p.submit(o);
  p.tick(0);
}

}  // namespace syg::organs
