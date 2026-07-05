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
  struct snapshot {  // block -> frame: last completed block's value
    const float* src_buf;  // producer's block buffer base
    std::unique_ptr<float> cell;
  };

  std::vector<block_node> blocks;  // fused per-node segments, topo order
  std::vector<frame_node> frames;
  std::vector<latch> latches;
  std::vector<snapshot> snapshots;
  std::vector<std::pair<std::size_t, std::size_t>> frame_edges;  // src, dst
  std::vector<std::unique_ptr<std::vector<float>>> latch_bufs;
  std::vector<float> silence;
  std::map<std::string, std::vector<float>> param_bufs;  // route -> buffer
  std::vector<edit_op> inlet;
  // the segment list (ADR-013): fused per-node block loops for feedforward
  // stretches, sample-interleaved loops for islands
  struct segment {
    bool island = false;
    std::vector<std::size_t> nodes;                 // block indices
    std::map<std::size_t, std::vector<std::size_t>> backward;  // node -> in-ports with z⁻¹
  };
  std::vector<segment> segments;
  struct block_edge { std::size_t src, dst, dst_port; };
  std::vector<block_edge> block_edges;
  long process_calls = 0;
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
      if (std::string(b.type->in_ports[i].discipline) == "value")
        continue;  // a param: set_num territory, not a stream buffer
      float v = 0.0f;
      if (doc.defaults.contains(route) && doc.defaults[route].is_number())
        v = doc.defaults[route];
      auto& buf = im->param_bufs[route];
      buf.assign(static_cast<std::size_t>(block), v);
      b.ins[i] = buf.data();
    }
  // wiring; inferred boundaries get latches (visible via regions())
  std::set<std::size_t> latched, snapped;
  for (const auto& m : regions.mappings) {
    if (m.mapping == "latch") latched.insert(m.edge);
    if (m.mapping == "snapshot") snapped.insert(m.edge);
  }
  for (std::size_t i = 0; i < doc.edges.size(); ++i) {
    const auto& [from, to] = doc.edges[i];
    auto sid = from.substr(0, from.find('/'));
    auto sport = from.substr(from.find('/') + 1);
    auto did = to.substr(0, to.find('/'));
    auto dport = to.substr(to.find('/') + 1);
    if (snapped.count(i)) {
      auto* sb = im->block_of(sid);
      auto* df = im->frame_of(did);
      if (!sb || !df) throw std::runtime_error("snapshot endpoints missing");
      auto& sn = im->snapshots.emplace_back();
      sn.src_buf = sb->out_bufs[port_index(sb->type->out_ports, sport)].data();
      sn.cell = std::make_unique<float>(0.0f);
      df->ins[port_index(df->type->in_ports, dport)] = sn.cell.get();
      continue;
    }
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
      auto dp_i = port_index(db->type->in_ports, dport);
      db->ins[dp_i] = sb->out_bufs[port_index(sb->type->out_ports, sport)].data();
      im->block_edges.push_back(
          {static_cast<std::size_t>(sb - im->blocks.data()),
           static_cast<std::size_t>(db - im->blocks.data()), dp_i});
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

  // --- segments (ADR-013): SCCs are per-sample islands ---
  // an explicit delay of at least a block breaks the cycle at its input
  auto delay_len = [&](const std::string& id) {
    auto it = doc.defaults.find(id + "/samples");
    return it != doc.defaults.end() && it->is_number() ? it->get<double>() : 0.0;
  };
  std::vector<std::vector<std::size_t>> adj(im->blocks.size());
  for (const auto& e : im->block_edges) {
    const auto& dst = im->blocks[e.dst];
    if (std::string(dst.type->name) == "delay" && delay_len(dst.id) >= block)
      continue;  // cut: the explicit delay decouples the loop
    adj[e.src].push_back(e.dst);
  }
  // Tarjan SCC (iterative-enough at this scale: recursive lambda)
  std::vector<int> comp(im->blocks.size(), -1), low(im->blocks.size()),
      num(im->blocks.size(), -1);
  std::vector<std::size_t> stk;
  std::vector<bool> on(im->blocks.size(), false);
  int counter = 0, ncomp = 0;
  auto dfs = [&](auto&& self, std::size_t v) -> void {
    num[v] = low[v] = counter++;
    stk.push_back(v);
    on[v] = true;
    for (auto w : adj[v]) {
      if (num[w] < 0) {
        self(self, w);
        low[v] = std::min(low[v], low[w]);
      } else if (on[w]) {
        low[v] = std::min(low[v], num[w]);
      }
    }
    if (low[v] == num[v]) {
      while (true) {
        auto w = stk.back();
        stk.pop_back();
        on[w] = false;
        comp[w] = ncomp;
        if (w == v) break;
      }
      ++ncomp;
    }
  };
  for (std::size_t v = 0; v < im->blocks.size(); ++v)
    if (num[v] < 0) dfs(dfs, v);
  // condensation topo order (Kahn), members kept in doc order
  std::vector<std::set<int>> cadj(static_cast<std::size_t>(ncomp));
  std::vector<int> indeg(static_cast<std::size_t>(ncomp), 0);
  for (const auto& e : im->block_edges) {
    if (comp[e.src] != comp[e.dst] &&
        cadj[static_cast<std::size_t>(comp[e.src])].insert(comp[e.dst]).second)
      ++indeg[static_cast<std::size_t>(comp[e.dst])];
  }
  std::vector<std::vector<std::size_t>> members(static_cast<std::size_t>(ncomp));
  for (std::size_t v = 0; v < im->blocks.size(); ++v)
    members[static_cast<std::size_t>(comp[v])].push_back(v);
  std::vector<int> ready;
  for (int c = 0; c < ncomp; ++c)
    if (indeg[static_cast<std::size_t>(c)] == 0) ready.push_back(c);
  std::vector<bool> self_loop(static_cast<std::size_t>(ncomp), false);
  for (const auto& e : im->block_edges)
    if (e.src == e.dst) self_loop[static_cast<std::size_t>(comp[e.src])] = true;
  while (!ready.empty()) {
    int c = ready.back();
    ready.pop_back();
    auto& seg = im->segments.emplace_back();
    seg.nodes = members[static_cast<std::size_t>(c)];
    seg.island = seg.nodes.size() > 1 || self_loop[static_cast<std::size_t>(c)];
    if (seg.island) {
      // a cycle may not pass through a block-override interior (EXE-10.3)
      for (auto v : seg.nodes)
        if (im->blocks[v].type->block_override) {
          std::string edge = "?";
          std::set<std::size_t> in_island(seg.nodes.begin(), seg.nodes.end());
          for (const auto& e : im->block_edges)
            if (e.dst == v && in_island.count(e.src))
              edge = im->blocks[e.src].id + "/out -> " + im->blocks[v].id +
                     "/" + im->blocks[v].type->in_ports[e.dst_port].name;
          throw std::runtime_error(
              "edit rejected: the cycle passes through block-override node '" +
              im->blocks[v].id + "'; the edge " + edge +
              " needs an explicit block-sized delay");
        }
      // backward in-island edges carry the one-sample z⁻¹
      std::map<std::size_t, std::size_t> order;
      for (std::size_t i = 0; i < seg.nodes.size(); ++i) order[seg.nodes[i]] = i;
      for (const auto& e : im->block_edges)
        if (order.count(e.src) && order.count(e.dst) &&
            order[e.src] >= order[e.dst])
          seg.backward[e.dst].push_back(e.dst_port);
    }
    for (auto d : cadj[static_cast<std::size_t>(c)])
      if (--indeg[static_cast<std::size_t>(d)] == 0) ready.push_back(d);
  }
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

long exec_plan::process_calls() const { return im_->process_calls; }

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
  // 4. the segments, RT-audited: fused feedforward loops; islands
  // sample-interleaved with z⁻¹ on backward edges (ADR-013)
  {
    abi::phase_guard g(abi::phase::process);
    for (auto& seg : im_->segments) {
      if (!seg.island) {
        for (auto v : seg.nodes) {
          auto& b = im_->blocks[v];
          b.type->process(b.state, b.ins.data(), b.outs.data(), block_);
          ++im_->process_calls;
        }
        continue;
      }
      thread_local std::vector<const float*> tin;
      thread_local std::vector<float*> tout;
      for (int i = 0; i < block_; ++i) {
        for (auto v : seg.nodes) {
          auto& b = im_->blocks[v];
          tin.resize(b.ins.size());
          tout.resize(b.outs.size());
          for (std::size_t k = 0; k < b.ins.size(); ++k) tin[k] = b.ins[k] + i;
          if (auto bw = seg.backward.find(v); bw != seg.backward.end())
            for (auto k : bw->second)  // z⁻¹: previous sample, wrapping to
              tin[k] = b.ins[k] + (i == 0 ? block_ - 1 : i - 1);  // last block
          for (std::size_t k = 0; k < b.outs.size(); ++k) tout[k] = b.outs[k] + i;
          b.type->process(b.state, tin.data(), tout.data(), 1);
          ++im_->process_calls;
        }
      }
    }
  }
  // 5. snapshots publish at period end: frame sees the completed block
  for (auto& sn : im_->snapshots) *sn.cell = sn.src_buf[block_ - 1];
  return im_->sink ? im_->sink : im_->silence.data();
}

}  // namespace syg::executor
