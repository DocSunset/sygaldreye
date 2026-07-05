#include "crown.hpp"

#include <cstdlib>
#include <stdexcept>

namespace syg::crown {

struct plan::instance {
  const native_type* type;
  void* state;
  std::vector<std::vector<float>> out_bufs;
  std::vector<const float*> ins;
  std::vector<float*> outs;
  escapement::node as_node() {
    return {state, ins.data(), outs.data(), type->process};
  }
};

plan::plan(std::vector<const native_type*> linked, int block)
    : linked_(std::move(linked)), block_(block), silence_(size_t(block), 0.0f) {}

plan::plan(plan&&) noexcept = default;

plan::~plan() {
  for (auto& [id, inst] : instances_) inst->type->destroy(inst->state);
}

void plan::submit(op o) { inlet_.push_back(std::move(o)); }

void plan::apply(const op& o) {
  using k = op::kind;
  switch (o.what) {
    case k::instantiate: {
      const native_type* t = nullptr;
      for (auto* n : linked_)
        if (o.b == n->name) t = n;
      if (!t) throw std::runtime_error("no linked native: " + o.b);
      auto inst = std::make_unique<instance>();
      inst->type = t;
      inst->state = t->create();
      inst->out_bufs.assign(t->out_ports.size(),
                            std::vector<float>(size_t(block_), 0.0f));
      instances_[o.a] = std::move(inst);
      order_.push_back({o.a, o.b});
      break;
    }
    case k::link:
      edges_.emplace_back(o.a, o.b);
      break;
    case k::unlink:
      std::erase(edges_, std::pair{o.a, o.b});
      break;
    case k::remove: {
      auto it = instances_.find(o.a);
      if (it == instances_.end()) throw std::runtime_error("no node: " + o.a);
      it->second->type->destroy(it->second->state);
      instances_.erase(it);
      std::erase_if(order_, [&](const record& r) { return r.id == o.a; });
      std::erase_if(edges_, [&](const auto& e) {
        return e.first.starts_with(o.a + "/") || e.second.starts_with(o.a + "/");
      });
      break;
    }
    case k::write_default: {
      auto slash = o.a.find('/');
      auto& inst = instances_.at(o.a.substr(0, slash));
      auto port = o.a.substr(slash + 1);
      char* end = nullptr;
      double num = std::strtod(o.b.c_str(), &end);
      if (end && *end == '\0' && end != o.b.c_str())
        inst->type->set_num(inst->state, port.c_str(), num);
      else
        inst->type->set_text(inst->state, port.c_str(), o.b.c_str());
      defaults_.emplace_back(o.a, o.b);
      break;
    }
  }
}

// Re-derive the escapement wiring from the mutable table (control-rate work).
void plan::rewire() {
  auto find_port = [](const std::vector<const char*>& ports, const std::string& p) {
    for (size_t i = 0; i < ports.size(); ++i)
      if (p == ports[i]) return i;
    throw std::runtime_error("no port: " + p);
  };
  for (auto& [id, inst] : instances_) {
    inst->ins.assign(inst->type->in_ports.size(), silence_.data());
    inst->outs.clear();
    for (auto& b : inst->out_bufs) inst->outs.push_back(b.data());
  }
  for (const auto& [from, to] : edges_) {
    auto s1 = from.find('/'), s2 = to.find('/');
    auto& src = instances_.at(from.substr(0, s1));
    auto& dst = instances_.at(to.substr(0, s2));
    auto op_ = find_port(src->type->out_ports, from.substr(s1 + 1));
    auto ip = find_port(dst->type->in_ports, to.substr(s2 + 1));
    dst->ins[ip] = src->out_bufs[op_].data();
  }
  ticklist_.clear();
  for (const auto& r : order_) ticklist_.push_back(instances_.at(r.id)->as_node());
}

void plan::tick(int frames) {
  if (!inlet_.empty()) {  // the tick boundary: apply, then rewire once
    for (const auto& o : inlet_) apply(o);
    inlet_.clear();
    rewire();
  }
  escapement::movement m{ticklist_.data(), static_cast<int>(ticklist_.size())};
  escapement::tick(m, frames);
}

const float* plan::input_buffer(const std::string& id,
                                const std::string& port) const {
  const auto& inst = instances_.at(id);
  for (size_t i = 0; i < inst->type->in_ports.size(); ++i)
    if (port == inst->type->in_ports[i]) return inst->ins[i];
  throw std::runtime_error("no port: " + id + "/" + port);
}

}  // namespace syg::crown
