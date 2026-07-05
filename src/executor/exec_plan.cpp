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
    std::vector<float> outs;        // value cells
    std::vector<const float*> ins;  // upstream cells
    bool clocked = false;
    bool dirty = true;
    long recomputed = 0;
    std::vector<float> in_vals;
  };
  struct latch {
    const float* cell;
    std::vector<float>* target;
  };

  std::vector<block_node> blocks;  // fused per-node segments, topo order
  std::vector<frame_node> frames;
  std::vector<latch> latches;
  std::vector<std::pair<std::size_t, std::size_t>> frame_edges;  // src, dst
  std::vector<std::unique_ptr<std::vector<float>>> latch_bufs;
  std::vector<float> silence;
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
  void set_param(const std::string& route, double v) {
    auto id = route.substr(0, route.find('/'));
    auto port = route.substr(route.find('/') + 1);
    if (auto pb = param_bufs.find(route); pb != param_bufs.end()) {
      pb->second.assign(pb->second.size(), static_cast<float>(v));
    } else if (auto* b = block_of(id)) {
      b->type->set_num(b->state, port.c_str(), v);
    } else if (auto* f = frame_of(id)) {
      f->type->set_num(f->state, port.c_str(), v);
      f->dirty = true;  // staleness propagates from the write (ADR-015)
    }
  }
  ~impl() {
    for (auto& b : blocks)
      if (b.state) b.type->destroy(b.state);
    for (auto& f : frames)
      if (f.state) f.type->destroy(f.state);
  }
};

std::unique_ptr<exec_plan::impl> exec_plan::build_impl(
    const organs::graph_doc& doc, const region_map& regions, int block) {
  auto im = std::make_unique<exec_plan::impl>();
  im->silence.assign(static_cast<std::size_t>(block), 0.0f);
  std::set<std::string> in_block(regions.block.begin(), regions.block.end());
  for (const auto& [id, tname] : doc.nodes) {
    const auto* t = type_named(tname);
    if (in_block.count(id)) {
      auto& b = im->blocks.emplace_back();
      b.id = id;
      b.type = t;
      b.state = t->create();
      b.out_bufs.assign(t->out_ports.size(),
                        std::vector<float>(static_cast<std::size_t>(block), 0.0f));
      b.ins.assign(t->in_ports.size(), im->silence.data());
    } else {
      auto& f = im->frames.emplace_back();
      f.id = id;
      f.type = t;
      f.state = t->create();
      f.outs.assign(t->out_ports.size(), 0.0f);
      f.ins.assign(t->in_ports.size(), nullptr);
      f.in_vals.assign(t->in_ports.size(), 0.0f);
      f.clocked = t->clocked;
    }
  }
  for (const auto& [route, v] : doc.defaults.items()) {
    auto id = route.substr(0, route.find('/'));
    auto port = route.substr(route.find('/') + 1);
    auto apply = [&](const crown::native_type* t, void* state) {
      if (v.is_number()) t->set_num(state, port.c_str(), v);
      else if (v.is_string())
        t->set_text(state, port.c_str(), v.get_ref<const std::string&>().c_str());
    };
    if (auto* b = im->block_of(id)) apply(b->type, b->state);
    else if (auto* f = im->frame_of(id)) apply(f->type, f->state);
  }
  // unconnected block in-ports take their DEFAULT as a constant buffer
  std::set<std::string> connected;
  for (const auto& [from, to] : doc.edges) connected.insert(to);
  for (auto& b : im->blocks)
    for (std::size_t i = 0; i < b.type->in_ports.size(); ++i) {
      std::string route = b.id + "/" + b.type->in_ports[i].name;
      if (connected.count(route)) continue;
      float v = 0.0f;
      if (doc.defaults.contains(route) && doc.defaults[route].is_number())
        v = doc.defaults[route];
      auto& buf = im->param_bufs[route];
      buf.assign(static_cast<std::size_t>(block), v);
      b.ins[i] = buf.data();
    }
  // wiring; inferred boundaries get latches (visible via regions())
  std::set<std::size_t> latched;
  for (const auto& m : regions.mappings)
    if (m.mapping == "latch") latched.insert(m.edge);
  for (std::size_t i = 0; i < doc.edges.size(); ++i) {
    const auto& [from, to] = doc.edges[i];
    auto sid = from.substr(0, from.find('/'));
    auto sport = from.substr(from.find('/') + 1);
    auto did = to.substr(0, to.find('/'));
    auto dport = to.substr(to.find('/') + 1);
    if (latched.count(i)) {
      auto* f = im->frame_of(sid);
      auto* b = im->block_of(did);
      if (!f || !b) throw std::runtime_error("latch endpoints missing");
      im->latch_bufs.push_back(std::make_unique<std::vector<float>>(
          static_cast<std::size_t>(block), 0.0f));
      auto* buf = im->latch_bufs.back().get();
      im->latches.push_back(
          {&f->outs[port_index(f->type->out_ports, sport)], buf});
      b->ins[port_index(b->type->in_ports, dport)] = buf->data();
    } else if (auto* db = im->block_of(did)) {
      auto* sb = im->block_of(sid);
      if (!sb) throw std::runtime_error("unmapped cross-region edge " + from);
      db->ins[port_index(db->type->in_ports, dport)] =
          sb->out_bufs[port_index(sb->type->out_ports, sport)].data();
    } else if (auto* df = im->frame_of(did)) {
      auto* sf = im->frame_of(sid);
      if (!sf) throw std::runtime_error("unmapped cross-region edge " + from);
      df->ins[port_index(df->type->in_ports, dport)] =
          &sf->outs[port_index(sf->type->out_ports, sport)];
      im->frame_edges.emplace_back(
          static_cast<std::size_t>(sf - im->frames.data()),
          static_cast<std::size_t>(df - im->frames.data()));
    }
  }
  for (auto& b : im->blocks)
    for (auto& ob : b.out_bufs) b.outs.push_back(ob.data());
  for (auto& b : im->blocks)
    if (b.type->out_ports.empty() && !b.ins.empty()) im->sink = b.ins[0];
  return im;
}

exec_plan::exec_plan(organs::graph_doc doc, int rate, int block)
    : doc_(std::move(doc)), regions_(infer_regions(doc_)), rate_(rate),
      block_(block) {
  if (!regions_.errors.empty())
    throw std::runtime_error("cannot realize: " + regions_.errors.front());
  im_ = build_impl(doc_, regions_, block_);
}

exec_plan::~exec_plan() = default;
exec_plan::exec_plan(exec_plan&&) noexcept = default;

void exec_plan::submit(edit_op o) { im_->inlet.push_back(std::move(o)); }

void exec_plan::undo() {
  if (cursor_ == 0) return;
  const auto& inv = log_[--cursor_].inverse;
  for (auto it = inv.rbegin(); it != inv.rend(); ++it) {
    auto o = *it;
    o.undo_replay = true;  // a cursor move, never new history
    im_->inlet.push_back(std::move(o));
  }
}

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

// structural ops mutate the doc; the plan re-realizes with migration
void exec_plan::apply_structural(const edit_op& o) {
  std::vector<edit_op> inverse;
  if (o.op == "add_node") {
    doc_.nodes.emplace_back(o.a, o.b);
    inverse.push_back({"remove_node", o.a, "", o.author});
  } else if (o.op == "remove_node") {
    std::string type;
    for (const auto& [id, t] : doc_.nodes)
      if (id == o.a) type = t;
    if (type.empty()) throw std::runtime_error("precondition: no node " + o.a);
    inverse.push_back({"add_node", o.a, type, o.author});
    for (const auto& [f, t] : doc_.edges)  // cascade, recorded for undo
      if (f.starts_with(o.a + "/") || t.starts_with(o.a + "/"))
        inverse.push_back({"add_edge", f, t, o.author});
    for (const auto& [route, v] : doc_.defaults.items())
      if (route.starts_with(o.a + "/") && v.is_number())
        inverse.push_back(
            {"set_param", route, std::to_string(v.get<double>()), o.author});
    std::erase_if(doc_.nodes, [&](const auto& n) { return n.first == o.a; });
    std::erase_if(doc_.edges, [&](const auto& e) {
      return e.first.starts_with(o.a + "/") || e.second.starts_with(o.a + "/");
    });
  } else if (o.op == "add_edge") {
    doc_.edges.emplace_back(o.a, o.b);
    inverse.push_back({"remove_edge", o.a, o.b, o.author});
  } else if (o.op == "remove_edge") {
    auto n = std::erase(doc_.edges, std::pair{o.a, o.b});
    if (!n) throw std::runtime_error("precondition: no edge " + o.a);
    inverse.push_back({"add_edge", o.a, o.b, o.author});
  } else {
    throw std::runtime_error("unknown edit op: " + o.op);
  }
  if (!o.undo_replay) {
    log_.push_back({o, std::move(inverse)});
    cursor_ = log_.size();
  }
}

void exec_plan::rebuild() {
  regions_ = infer_regions(doc_);
  if (!regions_.errors.empty())
    throw std::runtime_error("cannot realize: " + regions_.errors.front());
  auto fresh = build_impl(doc_, regions_, block_);
  // migrate state by route: same id, same type -> the old state comes home
  for (auto& n : fresh->blocks)
    if (auto* old = im_->block_of(n.id); old && old->type == n.type) {
      n.type->destroy(n.state);
      n.state = old->state;
      old->state = nullptr;
    }
  for (auto& n : fresh->frames)
    if (auto* old = im_->frame_of(n.id); old && old->type == n.type) {
      n.type->destroy(n.state);
      n.state = old->state;
      old->state = nullptr;
      n.recomputed = old->recomputed;
    }
  fresh->t = im_->t;
  fresh->next_frame = im_->next_frame;
  im_ = std::move(fresh);
  // defaults re-applied by build; the param journal replays the live edits
  for (const auto& [route, v] : param_journal_)
    im_->set_param(route, std::strtod(v.c_str(), nullptr));
}

const float* exec_plan::pump_block() {
  // 1. the boundary: drain the inlet
  bool structural = false;
  auto pending = std::move(im_->inlet);
  im_->inlet.clear();
  for (const auto& o : pending) {
    if (o.op == "set_param") {
      im_->set_param(o.a, std::strtod(o.b.c_str(), nullptr));
      std::string prev = "0";
      if (param_journal_.count(o.a))
        prev = param_journal_[o.a];
      else if (doc_.defaults.contains(o.a) && doc_.defaults[o.a].is_number())
        prev = std::to_string(doc_.defaults[o.a].get<double>());
      if (!o.undo_replay) {
        log_.push_back({o, {{"set_param", o.a, prev, o.author}}});
        cursor_ = log_.size();
      }
      param_journal_[o.a] = o.b;
    } else {
      apply_structural(o);
      structural = true;
    }
  }
  if (structural) rebuild();
  // 2. frame tick when due
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
  // 4. the block segments, RT-audited
  {
    abi::phase_guard g(abi::phase::process);
    for (auto& b : im_->blocks)
      b.type->process(b.state, b.ins.data(), b.outs.data(), block_);
  }
  return im_->sink ? im_->sink : im_->silence.data();
}

}  // namespace syg::executor
