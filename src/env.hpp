#pragma once
// env.hpp — the environment: a lexical frame of grips (LISP's environment,
// rediscovered). HERE is the only thing handed out of band; everything else —
// the content store, the ref table, arenas, devices — is an ORGAN: a live
// object gripped by a symbol at a decreed name. Organs are dumb (no chains);
// the environment is the ONLY thing that composes — every lookup walks
// frames. Three contracts, three verb sets, one dictionary shape (structure
// never enforced a contract; the API split does):
//   content  FIXED     key = hash(value); immutable, portable, verified dedup
//   refs     LIVE      name ↦ address; rebindable meaning; shippable (signed, later)
//   symbols  SITUATED  name ↦ grip on live memory; local, mortal, NEVER ships
#include "types.hpp"
#include <stdexcept>
#include <unordered_map>

struct syg_env_t {
  struct id_hash {  // an id IS a hash — hash it with a straight face
    std::size_t operator()(syg_hash h) const { return h.digest; }
  };
  syg_env_t* parent;  // nullptr at the floor
  std::unordered_map<syg_hash, syg_handle_t, id_hash> symbols;  // name ↦ grip
};

namespace syg {

// ── typed keys: the compiler keeps the keyspaces apart ──────────────────────
struct content_id { syg_hash h; };  // = hash(content) — fixed forever
struct ref_name   { syg_hash h; };  // conferred/derived — rebindable meaning
struct sym_name   { syg_hash h; };  // conferred/derived — a local grip's name

// an ADDRESS: what a ref may point at — either world, tagged. (In-system this
// is variant{content_id, sym_name}; this struct is its authored twin.) A ref
// whose address is `symbol` is LOCAL-ONLY — the mortal world never ships.
struct address { enum tag_t : std::uint8_t { content, symbol } tag; syg_hash h; };

// derived names — hash_mix(relation, subject): metadata attaches to any node
// without touching its bytes. (Mesh-boundary note: when FOREIGN content can
// drive local derived bindings, make the key the exact pair — or be on a
// crypto hash — lest a forged subject id alias two bindings.)
inline syg_hash derived(syg_hash relation, syg_hash subject) { return syg_hash::mix(relation, subject); }

// ── the organs (dumb: a bare map each, no composition) ──────────────────────
struct content_store { std::unordered_map<syg_hash, syg_handle_t, syg_env_t::id_hash> map; };
struct ref_store     { std::unordered_map<syg_hash, address,      syg_env_t::id_hash> map; };

inline const sym_name CONTENT{fiat("content").id};  // organ names, decreed
inline const sym_name REFS{fiat("refs").id};

// ── the environment's own verbs: a lexical frame's, nothing more ────────────
// A symbol grip's id IS its name — nothing was hashed; loud and deliberate.
inline void wire(syg_env_t* env, sym_name name, syg_handle_t grip) {
  grip.id = name.h;
  grip.env = env;
  env->symbols[name.h] = grip;  // rebind = re-wire = live-patch HERE
}
inline void unwire(syg_env_t* env, sym_name name) { env->symbols.erase(name.h); }
inline const syg_handle_t* find(const syg_env_t* env, sym_name name) {
  for (const syg_env_t* e = env; e; e = e->parent)
    if (auto it = e->symbols.find(name.h); it != e->symbols.end()) return &it->second;
  return nullptr;
}

// ── compositions: sugar over organs found by name, never primitives ─────────
// get: walk frames; ask EVERY frame's content organ (a nearer organ overlays,
// it does not shadow — content can't disagree, only be elsewhere).
inline const syg_handle_t* get(const syg_env_t* env, content_id id) {
  for (const syg_env_t* e = env; e; e = e->parent)
    if (auto it = e->symbols.find(CONTENT.h); it != e->symbols.end()) {
      auto& map = ((content_store*)it->second.data)->map;
      if (auto f = map.find(id.h); f != map.end()) return &f->second;
    }
  return nullptr;
}

// insert_or_get: VERIFIED dedup — on an id match, compare bytes. Equal ⇒ a
// true duplicate (no-op). Unequal ⇒ a 2⁻⁶⁴ miracle or an adversary; either
// way stop and yell — silent merge is impossible, not unlikely. New content
// is COPIED into the nearest organ (the store owns its bytes) and stamped
// with the organ's frame, so residents never outlive their store.
inline syg_handle_t insert_or_get(syg_env_t* env, const syg_handle_t& h) {
  if (const syg_handle_t* r = get(env, content_id{h.id})) {
    if (r->size != h.size || std::memcmp(r->data, h.data, h.size))
      throw std::runtime_error("syg: id collision — distinct content, same hash");
    return *r;
  }
  const syg_handle_t* organ = find(env, CONTENT);
  if (!organ) throw std::runtime_error("syg: no content organ wired here");
  void* copy = std::malloc(h.size);
  std::memcpy(copy, h.data, h.size);
  return ((content_store*)organ->data)
      ->map.emplace(h.id, syg_handle_t{.data = copy, .type = h.type, .id = h.id,
                                       .size = h.size, .env = organ->env})
      .first->second;
}

// bind: a name changes its mind — into the nearest ref organ.
inline void bind(syg_env_t* env, ref_name name, address a) {
  const syg_handle_t* organ = find(env, REFS);
  if (!organ) throw std::runtime_error("syg: no ref organ wired here");
  ((ref_store*)organ->data)->map[name.h] = a;
}
// follow: resolve the name HERE-outward; then the ADDRESS from HERE (not from
// where the binding was found) — into whichever world it names.
inline const syg_handle_t* follow(const syg_env_t* env, ref_name name) {
  for (const syg_env_t* e = env; e; e = e->parent)
    if (auto it = e->symbols.find(REFS.h); it != e->symbols.end()) {
      auto& map = ((ref_store*)it->second.data)->map;
      if (auto f = map.find(name.h); f != map.end())
        return f->second.tag == address::content ? get(env, content_id{f->second.h})
                                                 : find(env, sym_name{f->second.h});
    }
  return nullptr;
}

// ── minting through an environment ──────────────────────────────────────────
// register a freshly built term handle and free the temporary (types.hpp
// terms are malloc'd; the registered copy is the durable one).
inline syg_handle_t mint(syg_env_t* env, syg_handle_t h) {
  syg_handle_t r = insert_or_get(env, h);
  std::free(h.data);
  return r;
}
// the authored conveniences — mint AND register the name, so every hash the
// term carries decodes here.
inline syg_handle_t atom(syg_env_t* env, const char* name,
                         std::uint64_t size, std::uint64_t align) {
  return mint(env, atom(insert_or_get(env, symbol_node(name)).id, size, align));
}
inline syg_handle_t scope(syg_env_t* env, syg_hash parent, const char* name) {
  return mint(env, scope(parent, insert_or_get(env, symbol_node(name)).id));
}

// ── the floor: a fresh ground frame, organs wired, the decree registered ────
// The organ grips are typed GROUND — their decoders are out of band, which is
// exactly what GROUND means; behavior-as-relations later removes the casts.
inline syg_env_t* floor() {
  auto* env = new syg_env_t{nullptr, {}};
  wire(env, CONTENT, {.data = new content_store, .type = GROUND, .id = {}, .size = sizeof(content_store), .env = nullptr});
  wire(env, REFS,    {.data = new ref_store, .type = GROUND, .id = {}, .size = sizeof(ref_store), .env = nullptr});
  for (const syg_handle_t& h : {ATOM, STRUCTURE, VARIANT, MUTABLE_PTR, CONSTANT_PTR, SCOPE,
                                SYMBOL, symbol_node("string"), STRING})
    insert_or_get(env, h);
  return env;
}

}  // namespace syg
