#pragma once
// env.hpp — the environment: ONE pair-keyed table of handles, plus a parent.
// HERE is the only out-of-band handout; the content store is an ORGAN
// gripped in the same table; only the environment composes (lookups walk
// frames). A row is either a GRIP (data points at live memory — local,
// mortal, NEVER ships) or a REFERENCE (type REF, data null, the target
// riding in .id — a pure value, the shippable kind). The type column is the
// customs officer. Construction lives HERE: a node is BORN registered — the
// sole exception is types.hpp's inscribe_symbol (the decree precedes every
// environment).
//
// OWNERSHIP LAW: a syg_handle_t NEVER owns what data points at. Stores own
// content bytes; arenas own instances; static storage owns literals and word
// cells; REF rows own nothing by construction. Destruction is an owner's
// act, never a side effect of assignment. (Table destruction-ownership —
// unwire ticking an ERASE word by grip type — designed, not yet built;
// floor organs are immortal.)
#include "types.hpp"
#include <cstring>
#include <span>
#include <stdexcept>
#include <string_view>
#include <unordered_map>
#include <vector>

// keys are EXACT pairs — the fold survives only on bucket duty, where
// one-way is a virtue. Collisions cannot alias rows; coordinate queries
// (every sense of a designator, every name under a context) are filters.
struct grip_key {
  syg_hash context;     // GROUND | a scope | a relation
  syg_hash designator;  // a name id, a type id — the second coordinate
  constexpr bool operator==(const grip_key&) const = default;
};
struct syg_env_t {
  struct key_hash {
    std::size_t operator()(grip_key k) const { return syg_hash::mix(k.context, k.designator).digest; }
  };
  syg_env_t* parent;  // nullptr at the floor
  std::unordered_map<grip_key, syg_handle_t, key_hash> table;  // the WHOLE environment
};

namespace syg {

// ── the content organ (dumb: a bare map, no composition) ────────────────────
struct content_store {
  struct id_hash { std::size_t operator()(syg_hash h) const { return h.digest; } };
  std::unordered_map<syg_hash, syg_handle_t, id_hash> map;
};

// REF — the atom whose instances point into the content store. Its id is
// computable WITHOUT an env (term hashed on the stack), so find() can
// recognize REF rows const-ly; floor mints the resident so the hash decodes.
inline syg_hash ref_type_id() {
  atom_term t{.name = name_id("ref"), .size = 8, .align = 8};
  return syg_id(ATOM.id, sizeof t, &t);
}

// ── the environment's verbs ─────────────────────────────────────────────────
// wire stamps env ONLY. The key carries naming; a row's id keeps the
// handle's one true meaning — WHAT THIS REFERS TO ({} if nothing).
inline void wire(syg_env_t* env, syg_hash ctx, syg_hash name, syg_handle_t grip) {
  grip.env = env;
  env->table[{ctx, name}] = grip;  // rebind = re-wire = a name changing its mind
}
inline void unwire(syg_env_t* env, syg_hash ctx, syg_hash name) { env->table.erase({ctx, name}); }

// peek: the raw row, HERE-outward — for rebind tooling and serializers.
inline const syg_handle_t* peek(const syg_env_t* env, syg_hash ctx, syg_hash name) {
  for (const syg_env_t* e = env; e; e = e->parent)
    if (auto it = e->table.find({ctx, name}); it != e->table.end()) return &it->second;
  return nullptr;
}

// get: resolve a content id — walk frames, ask every frame's content organ
// (a nearer organ overlays, it does not shadow — content can't disagree,
// only be elsewhere). Declared here; body below (find precedes it).
inline const syg_handle_t* get(const syg_env_t* env, syg_hash id);

// find: resolve HERE-outward; a REF row derefs through the content store
// (from HERE, not from the frame that held the row) — callers ALWAYS get a
// real handle. One level, deliberately.
inline const syg_handle_t* find(const syg_env_t* env, syg_hash ctx, syg_hash name) {
  const syg_handle_t* row = peek(env, ctx, name);
  if (row && row->type == ref_type_id()) return get(env, row->id);
  return row;
}

// bind = wire a REFERENCE: a pure value row — no bytes, no allocation,
// nothing owned; the shippable kind.
inline void bind(syg_env_t* env, syg_hash ctx, syg_hash name, syg_hash target) {
  wire(env, ctx, name, {.data = nullptr, .type = ref_type_id(), .id = target, .size = 0,
                        .env = nullptr});
}

// ── fixed verbs ─────────────────────────────────────────────────────────────
inline const syg_handle_t* get(const syg_env_t* env, syg_hash id) {
  for (const syg_env_t* e = env; e; e = e->parent)
    if (auto it = e->table.find({GROUND, CONTENT.id}); it != e->table.end()) {
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
  const syg_handle_t* organ = peek(env, GROUND, CONTENT.id);
  if (!organ) throw std::runtime_error("syg: no content organ wired here");
  void* copy = std::malloc(h.size);
  std::memcpy(copy, h.data, h.size);
  return ((content_store*)organ->data)
      ->map.emplace(h.id, syg_handle_t{.data = copy, .type = h.type, .id = h.id,
                                       .size = h.size, .env = organ->env})
      .first->second;
}

// ── constructors: a node is BORN registered ─────────────────────────────────
// The one birth: term in caller storage → hash → insert copies. No mallocs
// here, no cheques; the temporary dies at return.
inline syg_handle_t node(syg_env_t* env, syg_hash type, const void* term, std::uint64_t n) {
  return insert_or_get(env, {.data = const_cast<void*>(term), .type = type,
                             .id = syg_id(type, n, term), .size = n, .env = nullptr});
}

inline syg_handle_t atom(syg_env_t* env, syg_hash name /* a string or SCOPE id */,
                         std::uint64_t size, std::uint64_t align) {
  atom_term t{.name = name, .size = size, .align = align};
  return node(env, ATOM.id, &t, sizeof t);
}

// canon_type: RE-DERIVE from the decree — mint-or-get, idempotent. Not a
// lookup: the caller holds the definition (CANON is compile-time data), and
// recomputing from knowledge is the system's native idiom.
inline syg_handle_t canon_type(syg_env_t* env, std::string_view name) {
  for (const canon_row& r : CANON)
    if (r.name == name) {
      if (r.name != "str") {                                // str's spelling is fiat, inscribed
        canon_type(env, "str");                             // the spelling's TYPE decodes first
        node(env, STR_TYPE, r.name.data(), r.name.size());  // the spelling decodes
      }
      atom_term t = r.term();
      return node(env, ATOM.id, &t, sizeof t);              // the term, as decreed
    }
  throw std::runtime_error("syg: not a canonical type");
}
template <class T> syg_handle_t canon_type(syg_env_t* env)   // the kind-mapper's entry
  { return canon_type(env, canon_name<T>()); }

// named helpers — canon delegates, and the local (non-canon, situated,
// never-ships) trio, minted on demand as before:
inline syg_handle_t string_type(syg_env_t* env) { return canon_type(env, "str"); }
inline syg_handle_t u64_type(syg_env_t* env)    { return canon_type(env, "u64"); }
inline syg_handle_t h64_type(syg_env_t* env)    { return canon_type<syg_hash>(env); }
inline syg_handle_t ref_type(syg_env_t* env)    { return canon_type(env, "ref"); }

// a string node — also every NAME: a name is a string used in a naming position.
inline syg_handle_t string_node(syg_env_t* env, const char* s)
  { return node(env, string_type(env).id, s, std::char_traits<char>::length(s)); }

inline syg_handle_t envptr_type(syg_env_t* env)
  { return atom(env, string_node(env, "envptr").id, sizeof(void*), alignof(void*)); }
inline syg_handle_t handle_type(syg_env_t* env)
  { return atom(env, string_node(env, "handle").id, sizeof(syg_handle_t), alignof(syg_handle_t)); }
inline syg_handle_t rest_handles_type(syg_env_t* env)
  { return atom(env, string_node(env, "rest_handles").id, 0, 1); }
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
  { return scope(env, parent, string_node(env, name).id); }

// ── behavior: ONE ABI, ONE discipline ───────────────────────────────────────
// THE word — void(*)(void**), IDENTICAL to the escapement's. A frame is
// READ-ONLY structure; each slot points at a payload and the word writes
// outputs THROUGH its slot. Slots follow the signature's declaration order,
// inputs then outputs; arity is SIGNATURE knowledge, never ABI knowledge.
using word = void (*)(void** argv);

// a method node: a grip on a word cell whose TYPE IS ITS SIGNATURE — an
// anonymous structure node of (name, type) fields: the shape reflection
// reads off a C++ function. id EMPTY: a fn pointer is a location, not
// content; when methods ship, their SOURCE ships, not these cells.
inline syg_handle_t method_node(syg_env_t* env, word* cell, syg_hash signature) {
  return {.data = cell, .type = signature, .id = {}, .size = sizeof *cell, .env = env};
}
// binding a method is ONE wire: context = the relation, designator = the
// type. "The CONSTRUCT of ATOM is this word cell."
inline void bind_method(syg_env_t* env, syg_hash relation, syg_hash type, syg_handle_t method) {
  wire(env, relation, type, method);
}

// resolve: the COLD half — ONE find. Cacheable: a binding IS a resolved
// method; the hot loop never walks a table. Exact-match today; the seam-0
// query engine slots in behind this signature later.
struct method { word fn; const syg_handle_t* sig; };
inline method resolve(const syg_env_t* env, syg_hash relation, syg_hash subject) {
  const syg_handle_t* m = find(env, relation, subject);
  if (!m) return {};
  return { *(word*)m->data, get(env, m->type) };   // the fn, and how to call it
}

// call: the ONE marshaller — handles in, raw frame out, driven by the
// method's signature:
//   hash64 field  ⇒ the arg IS a reference: pass &arg.id
//   envptr field  ⇒ HERE rides as a raw cell: pass &env
//   rest_handles  ⇒ TWO slots: the remaining-arg count (synthesized, never a
//                   minted node), then the caller's handle array itself —
//                   zero copies, and live grips (idless) ride through intact
//   last field    ⇒ the one output (stage-0 convention): pass &out
//   otherwise     ⇒ pass arg.data (the payload cell)
// Words never see a handle unless their signature declares one.
inline syg_handle_t call(syg_env_t* env, const method& m,
                         std::uint64_t n, const syg_handle_t* args) {
  if (!m.sig) throw std::runtime_error("syg: method signature not resident");
  auto* f = (const field_term*)((const char*)m.sig->data + sizeof(syg_hash));
  std::uint64_t nf = (m.sig->size - sizeof(syg_hash)) / sizeof(field_term);

  syg_hash h64 = h64_type(env).id, envp = envptr_type(env).id,
           rh  = rest_handles_type(env).id;
  syg_handle_t out{.data = nullptr, .type = {}, .id = {}, .size = 0, .env = env};
  std::uint64_t argc = 0;                        // the synthesized count cell
  std::vector<void*> frame;
  for (std::uint64_t i = 0, a = 0; i < nf; ++i) {
    if      (i == nf - 1)       frame.push_back(&out);
    else if (f[i].type == envp) frame.push_back(&env);
    else if (f[i].type == h64)  frame.push_back((void*)&args[a++].id);
    else if (f[i].type == rh)   { argc = n - a; frame.push_back(&argc);
                                  frame.push_back((void*)(args + a)); a = n; }
    else                        frame.push_back(args[a++].data);
  }
  m.fn(frame.data());
  return out;
}

// dispatch: resolve + call, over ANY relation. A boot-tape record is
// (relation, subject, arg-ids…): the crown's applier is this in a loop.
inline syg_handle_t dispatch(syg_env_t* env, syg_hash relation, syg_hash subject,
                             std::uint64_t n, const syg_handle_t* args) {
  method m = resolve(env, relation, subject);
  if (!m.fn) throw std::runtime_error("syg: no method bound for (relation, subject)");
  return call(env, m, n, args);
}
// emplace_or_get: mint BY TYPE ID — the constructors' entry point, one line.
inline syg_handle_t emplace_or_get(syg_env_t* env, syg_hash type,
                                   std::uint64_t n, const syg_handle_t* args)
  { return dispatch(env, CONSTRUCT, type, n, args); }

// generated-shape shims. A reflection-enabled registration TU will emit
// these from describe_function and DELETE the hand copies; the frame layouts
// below are exactly what it will produce:
inline void atom_construct_word(void** argv) {       // (env, name, size, align) → handle
  *(syg_handle_t*)argv[4] = atom(*(syg_env_t**)argv[0], *(syg_hash*)argv[1],
                                 *(std::uint64_t*)argv[2], *(std::uint64_t*)argv[3]);
}
inline void structure_construct_word(void** argv) {  // (env, args: name,(fname,ftype)…) → handle
  syg_env_t*          env  = *(syg_env_t**)argv[0];
  std::uint64_t       argc = *(std::uint64_t*)argv[1];
  const syg_handle_t* args = (const syg_handle_t*)argv[2];
  std::vector<field_term> f;
  for (std::uint64_t i = 1; i + 1 < argc; i += 2)
    f.push_back({args[i].id, args[i + 1].id});
  *(syg_handle_t*)argv[3] = structure(env, args[0].id, f);
}
// variant/pointer/scope folds: same shapes; bound when first needed.
inline word atom_ctor_cell      = atom_construct_word;      // static homes for the
inline word structure_ctor_cell = structure_construct_word; // method-node grips

// ── the floor: the organ wired, the decree enrolled, first citizens minted ──
inline void inscribe(syg_env_t* env, std::span<const syg_handle_t> nodes)
  { for (const syg_handle_t& h : nodes) insert_or_get(env, h); }

inline syg_env_t* floor() {
  auto* env = new syg_env_t{nullptr, {}};
  wire(env, GROUND, CONTENT.id, {.data = new content_store, .type = GROUND, .id = {},
                                 .size = sizeof(content_store), .env = nullptr});
  inscribe(env, ROSTER);                              // the decree becomes resident
  for (const canon_row& r : CANON) canon_type(env, r.name);   // the canon becomes resident
  string_node(env, "construct");                      // relations decode too
  // constructor signatures: ANONYMOUS structure nodes (identical signatures
  // unify by content). A registration TU will reflect these off the C++.
  syg_hash E = envptr_type(env).id, H = h64_type(env).id, U = u64_type(env).id,
           HN = handle_type(env).id, RH = rest_handles_type(env).id;
  auto sym = [&](const char* s) { return string_node(env, s).id; };
  field_term atom_sig[] = {{sym("env"), E}, {sym("name"), H}, {sym("size"), U},
                           {sym("align"), U}, {sym("out"), HN}};
  bind_method(env, CONSTRUCT, ATOM.id,
              method_node(env, &atom_ctor_cell, structure(env, GROUND, atom_sig).id));
  field_term struct_sig[] = {{sym("env"), E}, {sym("args"), RH}, {sym("out"), HN}};
  bind_method(env, CONSTRUCT, STRUCTURE.id,
              method_node(env, &structure_ctor_cell, structure(env, GROUND, struct_sig).id));
  return env;
}

}  // namespace syg
