#pragma once
// env.hpp — the environment: a lexical frame of grips. HERE is the only
// out-of-band handout; stores are ORGANS gripped at decreed names; only the
// environment composes (every lookup walks frames). Three contracts, three
// verb sets: content (FIXED), refs (LIVE), symbols (SITUATED). Construction
// lives HERE: a node is BORN registered — the sole exception is types.hpp's
// inscribe_symbol (the decree precedes every environment). Typed key
// wrappers are gone: the verb you call already says how a hash is read.
//
// OWNERSHIP LAW: a syg_handle_t NEVER owns what data points at. Stores own
// content bytes; arenas own instances; static storage owns literals and word
// cells. Destruction is an owner's act, never a side effect of assignment.
// (Symbol-table destruction-ownership — unwire ticking an ERASE word by
// grip type — is designed but not yet built; floor organs are immortal.)
#include "types.hpp"
#include <cstring>
#include <span>
#include <stdexcept>
#include <string_view>
#include <unordered_map>
#include <vector>

struct syg_env_t {
  struct id_hash {  // an id IS a hash — hash it with a straight face
    std::size_t operator()(syg_hash h) const { return h.digest; }
  };
  syg_env_t* parent;                                            // nullptr at the floor
  std::unordered_map<syg_hash, syg_handle_t, id_hash> symbols;  // name ↦ grip
};

namespace syg {

// an ADDRESS: what a ref points at — the one genuinely dynamic distinction.
// A symbol address is LOCAL-ONLY: the mortal world never ships.
struct address { enum tag_t : std::uint8_t { content, symbol } tag; syg_hash h; };

// derived names — mix(relation, subject): metadata attaches to any node
// without touching its bytes. (Mesh-boundary note: before FOREIGN content
// can drive local derived bindings, make the key the exact pair — or be on
// a crypto hash — lest a forged subject id alias two bindings.)
inline syg_hash derived(syg_hash relation, syg_hash subject) { return syg_hash::mix(relation, subject); }

// ── the organs (dumb: a bare map each, no composition) ──────────────────────
struct content_store { std::unordered_map<syg_hash, syg_handle_t, syg_env_t::id_hash> map; };
struct ref_store     { std::unordered_map<syg_hash, address,      syg_env_t::id_hash> map; };

// ── situated verbs: a lexical frame's, nothing more ─────────────────────────
inline void wire(syg_env_t* env, syg_hash name, syg_handle_t grip) {
  grip.id = name;    // a grip's id IS its name — nothing was hashed
  grip.env = env;
  env->symbols[name] = grip;  // rebind = re-wire = live-patch HERE
}
inline void unwire(syg_env_t* env, syg_hash name) { env->symbols.erase(name); }
inline const syg_handle_t* find(const syg_env_t* env, syg_hash name) {
  for (const syg_env_t* e = env; e; e = e->parent)
    if (auto it = e->symbols.find(name); it != e->symbols.end()) return &it->second;
  return nullptr;
}

// ── fixed verbs ─────────────────────────────────────────────────────────────
// get: walk frames; ask EVERY frame's content organ (a nearer organ overlays,
// it does not shadow — content can't disagree, only be elsewhere).
inline const syg_handle_t* get(const syg_env_t* env, syg_hash id) {
  for (const syg_env_t* e = env; e; e = e->parent)
    if (auto it = e->symbols.find(CONTENT.id); it != e->symbols.end()) {
      auto& map = ((content_store*)it->second.data)->map;
      if (auto f = map.find(id); f != map.end()) return &f->second;
    }
  return nullptr;
}
// insert_or_get: VERIFIED dedup — on an id match, compare bytes. Equal ⇒ a
// true duplicate (no-op). Unequal ⇒ a 2⁻⁶⁴ miracle or an adversary; either
// way stop and yell — silent merge is impossible, not unlikely. New content
// is COPIED into the nearest organ (the store owns its bytes) and stamped
// with the organ's frame, so residents never outlive their store.
inline syg_handle_t insert_or_get(syg_env_t* env, const syg_handle_t& h) {
  if (const syg_handle_t* r = get(env, h.id)) {
    if (r->size != h.size || std::memcmp(r->data, h.data, h.size))
      throw std::runtime_error("syg: id collision — distinct content, same hash");
    return *r;
  }
  const syg_handle_t* organ = find(env, CONTENT.id);
  if (!organ) throw std::runtime_error("syg: no content organ wired here");
  void* copy = std::malloc(h.size);
  std::memcpy(copy, h.data, h.size);
  return ((content_store*)organ->data)
      ->map.emplace(h.id, syg_handle_t{.data = copy, .type = h.type, .id = h.id,
                                       .size = h.size, .env = organ->env})
      .first->second;
}

// ── live verbs ──────────────────────────────────────────────────────────────
// bind: a name changes its mind — into the nearest ref organ.
inline void bind(syg_env_t* env, syg_hash name, address a) {
  const syg_handle_t* organ = find(env, REFS.id);
  if (!organ) throw std::runtime_error("syg: no ref organ wired here");
  ((ref_store*)organ->data)->map[name] = a;
}
// follow: resolve the name HERE-outward; then the ADDRESS from HERE (not
// from where the binding was found) — into whichever world it names.
inline const syg_handle_t* follow(const syg_env_t* env, syg_hash name) {
  for (const syg_env_t* e = env; e; e = e->parent)
    if (auto it = e->symbols.find(REFS.id); it != e->symbols.end()) {
      auto& map = ((ref_store*)it->second.data)->map;
      if (auto f = map.find(name); f != map.end())
        return f->second.tag == address::content ? get(env, f->second.h)
                                                 : find(env, f->second.h);
    }
  return nullptr;
}

// ── constructors: a node is BORN registered ─────────────────────────────────
// The one birth: term in caller storage → hash → insert copies. No mallocs
// here, no cheques; the temporary dies at return.
inline syg_handle_t node(syg_env_t* env, syg_hash type, const void* term, std::uint64_t n) {
  return insert_or_get(env, {.data = const_cast<void*>(term), .type = type,
                             .id = syg_id(type, n, term), .size = n, .env = nullptr});
}
inline syg_handle_t symbol_node(syg_env_t* env, const char* s)
  { return node(env, SYMBOL.id, s, std::char_traits<char>::length(s)); }

inline syg_handle_t atom(syg_env_t* env, syg_hash name /* a SYMBOL or SCOPE id */,
                         std::uint64_t size, std::uint64_t align) {
  atom_term t{.name = name, .size = size, .align = align};
  return node(env, ATOM.id, &t, sizeof t);
}

// resident type mints, on demand, idempotent (content addressing makes each
// a lookup after its first call) — each literal spelled exactly once:
inline syg_handle_t string_type(syg_env_t* env)
  { return atom(env, symbol_node(env, "string").id, DYNAMIC, 1); }
inline syg_handle_t u64_type(syg_env_t* env)
  { return atom(env, symbol_node(env, "uint64").id, 8, 8); }
inline syg_handle_t word_type(syg_env_t* env)
  { return atom(env, symbol_node(env, "word").id, sizeof(void*), alignof(void*)); }

inline syg_handle_t string_node(syg_env_t* env, const char* s)
  { return node(env, string_type(env).id, s, std::char_traits<char>::length(s)); }
inline syg_handle_t u64_node(syg_env_t* env, std::uint64_t v)
  { return node(env, u64_type(env).id, &v, sizeof v); }  // every "4" is one node

inline syg_handle_t composite(syg_env_t* env, const syg_handle_t& ctor, syg_hash name,
                              std::span<const field_term> fields) {
  std::vector<unsigned char> t(sizeof name + fields.size_bytes());
  std::memcpy(t.data(), &name, sizeof name);
  std::memcpy(t.data() + sizeof name, fields.data(), fields.size_bytes());
  return node(env, ctor.id, t.data(), t.size());
}
inline syg_handle_t structure(syg_env_t* env, syg_hash n, std::span<const field_term> f)
  { return composite(env, STRUCTURE, n, f); }
inline syg_handle_t variant(syg_env_t* env, syg_hash n, std::span<const field_term> c)
  { return composite(env, VARIANT, n, c); }
inline syg_handle_t mutable_ptr(syg_env_t* env, syg_hash p)
  { return node(env, MUTABLE_PTR.id, &p, sizeof p); }
inline syg_handle_t constant_ptr(syg_env_t* env, syg_hash p)
  { return node(env, CONSTANT_PTR.id, &p, sizeof p); }
inline syg_handle_t scope(syg_env_t* env, syg_hash parent, syg_hash name) {
  scope_term t{.parent = parent, .name = name};
  return node(env, SCOPE.id, &t, sizeof t);
}
inline syg_handle_t scope(syg_env_t* env, syg_hash parent, const char* name)
  { return scope(env, parent, symbol_node(env, name).id); }

// ── behavior: ONE ABI ───────────────────────────────────────────────────────
// FRAME CONTRACT: slots are NON-OWNING grips; a cold word may replace a slot
// freely — replacing a grip destroys nothing (ownership law above). Hot tick
// words use the other discipline: write THROUGH output cells, never
// reassign. The relation implies which applies.
using word = void (*)(syg_env_t*, std::uint64_t argc, syg_handle_t* argv);

// a method node: a grip on a word cell. id EMPTY on purpose: a fn pointer is
// a location, not content — ASLR re-rolls its bytes per run; wasm has no
// bytes at all. Locations get names (bind_method's derived name), never
// content-hashes; when methods ship, their SOURCE ships, not these cells.
inline syg_handle_t method_node(syg_env_t* env, word* cell) {
  return {.data = cell, .type = word_type(env).id, .id = {}, .size = sizeof *cell, .env = env};
}
// the ONE binding pattern — CONSTRUCT today; TICK, PLACE, ERASE verbatim
// later: grip the method at the derived name, point a ref at the grip.
inline void bind_method(syg_env_t* env, syg_hash relation, syg_hash type, syg_handle_t method) {
  syg_hash at = derived(relation, type);
  wire(env, at, method);
  bind(env, at, {address::symbol, at});
}
// emplace_or_get: mint BY TYPE ID — tick the type's constructor once. The
// frame is args + one return slot; the caller stops knowing term layouts.
inline syg_handle_t emplace_or_get(syg_env_t* env, syg_hash type,
                                   std::uint64_t argc, const syg_handle_t* argv) {
  const syg_handle_t* m = follow(env, derived(CONSTRUCT.id, type));
  if (!m) throw std::runtime_error("syg: no constructor bound for this type");
  std::vector<syg_handle_t> frame(argv, argv + argc);
  frame.push_back({});                            // the return slot
  (*(word*)m->data)(env, frame.size(), frame.data());
  return frame.back();
}

// native folds, wearing the ABI — each delegates to its typed constructor:
inline void atom_construct_word(syg_env_t* env, std::uint64_t argc, syg_handle_t* argv) {
  argv[argc - 1] = atom(env, argv[0].id,                    // name: a SYMBOL resident
                        *(std::uint64_t*)argv[1].data,      // size: a u64 VALUE resident
                        *(std::uint64_t*)argv[2].data);     // align
}
// composite fold: argv = [name, (field-name, field-type)…, out] — arity from argc.
inline void structure_construct_word(syg_env_t* env, std::uint64_t argc, syg_handle_t* argv) {
  std::vector<field_term> f;
  for (std::uint64_t i = 1; i + 1 < argc - 1; i += 2) f.push_back({argv[i].id, argv[i + 1].id});
  argv[argc - 1] = structure(env, argv[0].id, f);
}
// variant/pointer/scope folds: same shapes; bound when first needed.
inline word atom_ctor_cell      = atom_construct_word;      // static homes for the
inline word structure_ctor_cell = structure_construct_word; // method-node grips

// ── the floor: organs wired, the decree enrolled, first citizens minted ─────
inline void inscribe(syg_env_t* env, std::span<const syg_handle_t> nodes)
  { for (const syg_handle_t& h : nodes) insert_or_get(env, h); }

inline syg_env_t* floor() {
  auto* env = new syg_env_t{nullptr, {}};
  wire(env, CONTENT.id, {.data = new content_store, .type = GROUND, .id = {},
                         .size = sizeof(content_store), .env = nullptr});
  wire(env, REFS.id,    {.data = new ref_store, .type = GROUND, .id = {},
                         .size = sizeof(ref_store), .env = nullptr});
  inscribe(env, ROSTER);                              // the decree becomes resident
  string_type(env); u64_type(env); word_type(env);    // first citizens above it
  bind_method(env, CONSTRUCT.id, ATOM.id,      method_node(env, &atom_ctor_cell));
  bind_method(env, CONSTRUCT.id, STRUCTURE.id, method_node(env, &structure_ctor_cell));
  return env;
}

}  // namespace syg
