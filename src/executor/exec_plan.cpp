#include "exec_plan.hpp"

#include <cstdlib>
#include <set>
#include <stdexcept>

#include "phase.hpp"
#include "registry_face/registry_face.hpp"

namespace syg::executor {

namespace {

const crown::native_type* type_named(const std::string& n) {
  for (const auto* t : organs::registered_natives())
    if (n == t->name) return t;
  throw std::runtime_error("no linked native: " + n);
}

std::size_t port_index(const std::vector<crown::port_decl>& ports,
                       const std::string& name) {
  for (std::size_t i = 0; i < ports.size(); ++i)
    if (name == ports[i].name) return i;
  throw std::runtime_error("no port: " + name);
}

}  // namespace

struct exec_plan::impl {
  struct block_node {
    std::string id;
    const crown::native_type* type;
    void* state;
    std::vector<std::vector<float>> out_bufs;
    std::vector<const float*> ins;
    std::vector<float*> outs;
  };
  struct frame_node {
    std::string id;
    const crown::native_type* type;
    void* state;
    std::vector<float> outs;       // value cells
    std::vector<const float*> ins; // upstream cells (or defaults)
    bool clocked = false;          // wired to the frame clock (lfo: yes)
    bool dirty = true;
    long recomputed = 0;
    std::vector<float> in_vals;    // gathered per tick
  };
  struct latch {
    const float* cell;          // frame-side source
    std::vector<float>* target; // plan-owned buffer feeding the block side
  };

  std::vector<block_node> blocks;   // fused per-node segments, topo order
  std::vector<frame_node> frames;
  std::vector<latch> latches;
  std::vector<std::pair<std::size_t, std::size_t>> frame_edges;  // src, dst
  std::vector<std::unique_ptr<std::vector<float>>> latch_bufs;
  std::vector<float> silence, defaults_pool;
  std::map<std::string, float> param_cells;  // route -> default/param value
  std::map<std::string, std::vector<float>> param_bufs;  // route -> buffer
  std::vector<edit_op> inlet;
  double t = 0, next_frame = 0;
  const float* sink = nullptr;

  frame_node* frame_of(const std::string& id) {
    for (auto& f : frames)
      if (f.id == id) return &f;
    return nullptr;
  }
  block_node* block_of(const std::string& id) {
    for (auto& b : blocks)
      if (b.id == id) return &b;
    return nullptr;
  }
};

exec_plan::exec_plan(organs::graph_doc doc, int rate, int block)
    : im_(std::make_unique<impl>()), doc_(std::move(doc)),
      regions_(infer_regions(doc_)), rate_(rate), block_(block) {
  if (!regions_.errors.empty())
    throw std::runtime_error("cannot realize: " + regions_.errors.front());
  im_->silence.assign(static_cast<std::size_t>(block), 0.0f);
  std::set<std::string> in_block(regions_.block.begin(), regions_.block.end());
  // realize instances (doc order is topo order at this rung)
  for (const auto& [id, tname] : doc_.nodes) {
    const auto* t = type_named(tname);
    if (in_block.count(id)) {
      auto& b = im_->blocks.emplace_back();
      b.id = id;
      b.type = t;
      b.state = t->create();
      b.out_bufs.assign(t->out_ports.size(),
                        std::vector<float>(static_cast<std::size_t>(block), 0.0f));
      b.ins.assign(t->in_ports.size(), im_->silence.data());
    } else {
      auto& f = im_->frames.emplace_back();
      f.id = id;
      f.type = t;
      f.state = t->create();
      f.outs.assign(t->out_ports.size(), 0.0f);
      f.ins.assign(t->in_ports.size(), nullptr);
      f.in_vals.assign(t->in_ports.size(), 0.0f);
      f.clocked = t->clocked;  // wired to the executor clock (ADR-015)
    }
  }
  // defaults -> params (set_num / value cells)
  for (const auto& [route, v] : doc_.defaults.items()) {
    auto id = route.substr(0, route.find('/'));
    auto port = route.substr(route.find('/') + 1);
    if (auto* b = im_->block_of(id)) {
      if (v.is_number()) b->type->set_num(b->state, port.c_str(), v);
      else if (v.is_string())
        b->type->set_text(b->state, port.c_str(),
                          v.get_ref<const std::string&>().c_str());
    } else if (auto* f = im_->frame_of(id)) {
      if (v.is_number()) f->type->set_num(f->state, port.c_str(), v);
      else if (v.is_string())
        f->type->set_text(f->state, port.c_str(),
                          v.get_ref<const std::string&>().c_str());
    }
  }
  // unconnected block in-ports take their DEFAULT as a constant buffer
  std::set<std::string> connected;
  for (const auto& [from, to] : doc_.edges) connected.insert(to);
  for (auto& b : im_->blocks)
    for (std::size_t i = 0; i < b.type->in_ports.size(); ++i) {
      std::string route = b.id + "/" + b.type->in_ports[i].name;
      if (connected.count(route)) continue;
      float v = 0.0f;
      if (doc_.defaults.contains(route) && doc_.defaults[route].is_number())
        v = doc_.defaults[route];
      auto& buf = im_->param_bufs[route];
      buf.assign(static_cast<std::size_t>(block), v);
      b.ins[i] = buf.data();
    }
  // wire edges; inferred boundaries get latches (visible in regions())
  std::set<std::size_t> latched;
  for (const auto& m : regions_.mappings)
    if (m.mapping == "latch") latched.insert(m.edge);
  for (std::size_t i = 0; i < doc_.edges.size(); ++i) {
    const auto& [from, to] = doc_.edges[i];
    auto sid = from.substr(0, from.find('/'));
    auto sport = from.substr(from.find('/') + 1);
    auto did = to.substr(0, to.find('/'));
    auto dport = to.substr(to.find('/') + 1);
    if (latched.count(i)) {
      auto* f = im_->frame_of(sid);
      auto* b = im_->block_of(did);
      if (!f || !b) throw std::runtime_error("latch endpoints missing");
      im_->latch_bufs.push_back(std::make_unique<std::vector<float>>(
          static_cast<std::size_t>(block), 0.0f));
      auto* buf = im_->latch_bufs.back().get();
      im_->latches.push_back(
          {&f->outs[port_index(f->type->out_ports, sport)], buf});
      b->ins[port_index(b->type->in_ports, dport)] = buf->data();
    } else if (auto* db = im_->block_of(did)) {
      auto* sb = im_->block_of(sid);
      if (!sb) throw std::runtime_error("unmapped cross-region edge " + from);
      db->ins[port_index(db->type->in_ports, dport)] =
          sb->out_bufs[port_index(sb->type->out_ports, sport)].data();
    } else if (auto* df = im_->frame_of(did)) {
      auto* sf = im_->frame_of(sid);
      if (!sf) throw std::runtime_error("unmapped cross-region edge " + from);
      df->ins[port_index(df->type->in_ports, dport)] =
          &sf->outs[port_index(sf->type->out_ports, sport)];
      im_->frame_edges.emplace_back(
          static_cast<std::size_t>(sf - im_->frames.data()),
          static_cast<std::size_t>(df - im_->frames.data()));
    }
  }
  for (auto& b : im_->blocks)
    for (std::size_t i = 0; i < b.out_bufs.size(); ++i)
      b.outs.push_back(b.out_bufs[i].data());
  // the sink: the block region's dac-shaped node's input
  for (auto& b : im_->blocks)
    if (b.type->out_ports.empty() && !b.ins.empty()) im_->sink = b.ins[0];
}

exec_plan::~exec_plan() {
  if (!im_) return;
  for (auto& b : im_->blocks) b.type->destroy(b.state);
  for (auto& f : im_->frames) f.type->destroy(f.state);
}

exec_plan::exec_plan(exec_plan&&) noexcept = default;

void exec_plan::submit(edit_op o) { im_->inlet.push_back(std::move(o)); }

long exec_plan::recomputes(const std::string& id) const {
  for (const auto& f : im_->frames)
    if (f.id == id) return f.recomputed;
  return -1;
}

float exec_plan::value_of(const std::string& id_port) const {
  auto id = id_port.substr(0, id_port.find('/'));
  auto port = id_port.substr(id_port.find('/') + 1);
  for (const auto& f : im_->frames)
    if (f.id == id) return f.outs[port_index(f.type->out_ports, port)];
  throw std::runtime_error("no frame cell: " + id_port);
}

const float* exec_plan::pump_block() {
  // 1. the boundary: drain the inlet (param writes dirty their cone)
  for (const auto& o : im_->inlet) {
    if (o.op == "set_param") {
      auto id = o.a.substr(0, o.a.find('/'));
      auto port = o.a.substr(o.a.find('/') + 1);
      double v = std::strtod(o.b.c_str(), nullptr);
      if (auto pb = im_->param_bufs.find(o.a); pb != im_->param_bufs.end()) {
        pb->second.assign(pb->second.size(), static_cast<float>(v));
      } else if (auto* b = im_->block_of(id)) {
        b->type->set_num(b->state, port.c_str(), v);
      } else if (auto* f = im_->frame_of(id)) {
        f->type->set_num(f->state, port.c_str(), v);
        f->dirty = true;  // staleness propagates from the write (ADR-015)
      }
    } else {
      throw std::runtime_error("structural edits arrive with re-realize");
    }
  }
  im_->inlet.clear();
  // 2. frame tick when due (the frame clock demands its region)
  double block_dt = static_cast<double>(block_) / rate_;
  if (im_->t >= im_->next_frame) {
    double dt = 1.0 / 60.0;
    for (std::size_t fi = 0; fi < im_->frames.size(); ++fi) {
      auto& f = im_->frames[fi];
      if (!f.clocked && !f.dirty) continue;  // quiescent (EXE-11)
      if (f.type->value_tick) {
        for (std::size_t i = 0; i < f.ins.size(); ++i)
          f.in_vals[i] = f.ins[i] ? *f.ins[i] : 0.0f;
        f.type->value_tick(f.state, dt, f.in_vals.data(), f.outs.data());
        ++f.recomputed;
        for (const auto& [src, dst] : im_->frame_edges)  // dirty the cone
          if (src == fi) im_->frames[dst].dirty = true;
      }
      f.dirty = false;
    }
    im_->next_frame += dt;
  }
  im_->t += block_dt;
  // 3. latches apply at the block boundary, never mid-block (EXE-4.1)
  for (auto& l : im_->latches) l.target->assign(l.target->size(), *l.cell);
  // 4. the block segments: fused per-node loops, RT-audited
  {
    abi::phase_guard g(abi::phase::process);
    for (auto& b : im_->blocks)
      b.type->process(b.state, b.ins.data(), b.outs.data(), block_);
  }
  return im_->sink ? im_->sink : im_->silence.data();
}

}  // namespace syg::executor
