#pragma once
// syg.hpp — the type-erased node ABI (stage 0).
//
// POD, C-ABI structs a foreign .so could hand back, plus the runtime helpers over
// them. This is the shape the reflection layer (stage0.hpp) will GENERATE into; here
// it is authored by hand as the design's target. Sketch-stage — the reasoning lives
// in ../stage0.md. Not yet wired to the reflection layer.
#include <cstddef>
#include <cstdint>
#include <cstdlib>

// ── identity hashes: FNV-1a, 64-bit. A DEFINED function (NOT std::hash, which isn't
//    stable across implementations) so every peer computes the same value. Pinned —
//    a peer that hashes differently can't agree on a type. For runtime identity
//    compares only, not content-addressing (widen `shape` to crypto if that changes).
using syg_hash = std::uint64_t;
inline syg_hash syg_hash_bytes(const void* p, std::size_t n, syg_hash h = 0xcbf29ce484222325ull) {
  const unsigned char* b = static_cast<const unsigned char*>(p);
  for (std::size_t i = 0; i < n; ++i) { h ^= b[i]; h *= 0x100000001b3ull; }
  return h;
}
inline syg_hash syg_hash_str(const char* s) { return syg_hash_bytes(s, __builtin_strlen(s)); }
inline syg_hash syg_hash_mix(syg_hash a, syg_hash b) { return syg_hash_bytes(&b, sizeof b, a); }

extern "C" {
typedef struct syg_type_t syg_type_t;
typedef struct syg_meta_t syg_meta_t;
typedef struct syg_node_t syg_node_t;

// a typed value = a decoder (type) paired with an encoded blob (data). A cell, an
// edge payload, an inlet default, a static's storage are all instances of this.
struct syg_value_t { const syg_type_t* type; void* data; };

// a member = name + type + locator. Instance members are located by an offset into
// the object; static (shared, class-level) members by an absolute address.
struct syg_field_t  { const char* name; const syg_type_t* type; std::size_t offset; };
struct syg_static_t { const char* name; const syg_type_t* type; void* addr; };

// a TYPE: pure data description + RAII lifecycle. One static instance per type. A
// member is an INPUT iff its type is a pointer-to-const (const T*, borrowed edge);
// everything else is owned output/state — and "state" is just an output read back.
struct syg_type_t {
  syg_hash            id;              // syg_hash_mix(scope_hash, name_hash) — nominal identity
  syg_hash            name_hash;       // syg_hash_str(name)
  syg_hash            scope_hash;      // fold of the scope slugs
  syg_hash            shape;           // fold of flattened packed primitive leaves — byte fingerprint
  const char*         name;            // bare slug
  const char* const*  scope;           // enclosing namespaces, outermost first
  std::size_t         size;            // one instance's bytes
  std::size_t         member_count;
  const syg_field_t*  members;         // instance members (const T* => input; else output/state)
  std::size_t         static_count;
  const syg_static_t* static_members;
  void (*place)(void* mem, void** inputs);  // ctor + bind each const T* input to inputs[i]
  void (*erase)(void* obj);                 // dtor, no free
  void (*move)(void* dst, void* src);       // migrate: steal owned, copy borrowed
};

// a NODE = a type + the one thing data can't do: run.
struct syg_meta_t { const syg_type_t* type; void (*run)(void* self); };

// an INSTANCE: an intrusive meta header, then the component state.
struct syg_node_t { const syg_meta_t* meta; /* state (type->size bytes) follows the header */ };
}

using syg_meta = const syg_meta_t*;   // shared static class/vtable
using syg_node = syg_node_t*;         // instance handle — one pointer, passes as void*

// derived at runtime, never stored — the point of the split.
inline syg_meta          meta_of(syg_node n) { return *reinterpret_cast<syg_meta*>(n); }  // first member
inline const syg_type_t* type_of(syg_node n) { return meta_of(n)->type; }
inline void*             state(syg_node n)   { return reinterpret_cast<char*>(n) + sizeof(syg_meta); }

inline syg_node create(syg_meta m, void** inputs) {                            // = new: alloc + place
  syg_node n = static_cast<syg_node>(std::malloc(sizeof(syg_meta) + m->type->size));
  *reinterpret_cast<syg_meta*>(n) = m;                                         // write the vptr header
  m->type->place(state(n), inputs);
  return n;
}
inline void destroy(syg_node n) { type_of(n)->erase(state(n)); std::free(n); }  // = delete: erase + free
inline void tick(syg_node n)    { meta_of(n)->run(state(n)); }
