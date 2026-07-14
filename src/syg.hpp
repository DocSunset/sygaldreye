#pragma once
// syg.hpp — the type-erased type ABI (stage 0).
//
// POD, C-ABI structs a foreign .so could hand back: a type descriptor (identity +
// layout + RAII). This is the shape the reflection layer (stage0.hpp) generates into
// and the runtime type-family builders (syg::variant, …) mint at spawn. The reasoning
// lives in ../stage0.md.
#include "hash.hpp"   // syg_hash = hash64_fnv1a: decree-v1 identity (digest type + algorithm)
#include <cstddef>
#include <cstdint>

extern "C" {
typedef struct syg_type_t syg_type_t;

// a typed value = a decoder (type) paired with an encoded blob (data). A cell, an
// edge payload, an inlet default, a template value-arg's storage are all instances.
struct syg_value_t { const syg_type_t* type; void* data; };

// a member = name + type + locator: an offset into each instance.
struct syg_field_t { const char* name; const syg_type_t* type; std::size_t offset; };

// a template argument = what monomorphizes this type (constant<440>, latch<float>).
// Read the kind off `addr`: non-null ⇒ a VALUE arg, `type` describes the bytes at
// `addr`; null ⇒ a TYPE arg, `type` IS the argument and there is no storage. These
// are the type's "statics" reframed by role — its parameters, folded into params_hash.
struct syg_template_arg_t { const char* name; const syg_type_t* type; void* addr; };

// a TYPE: pure data description + RAII lifecycle. One static instance per type. A
// member is an INPUT iff its type is a pointer-to-const (const T*, borrowed edge);
// everything else is owned output/state — and "state" is just an output read back.
struct syg_type_t {
  syg_hash                  id;          // syg_hash_mix(scope_hash, name_hash, params_hash) — nominal identity
  syg_hash                  name_hash;   // syg_hash_str(name)
  syg_hash                  scope_hash;  // fold of the scope slugs
  syg_hash                  shape;       // fold of flattened packed primitive leaves — byte fingerprint
  const char*               name;        // bare slug
  const char* const*        scope;       // enclosing namespaces, outermost first
  std::size_t               size;        // one instance's bytes
  std::size_t               align;        // one instance's alignment (widest leaf) — for inline layout
  std::size_t               member_count;
  const syg_field_t*        members;     // instance members (const T* => input; else output/state)
  std::size_t               template_arg_count;
  const syg_template_arg_t* template_args;  // the args that monomorphize this type; fold into params_hash
  // The type's ONE behavior: its run/node step. A syg_value IS its own frame — its members
  // are the argument and return slots, so tick reads and writes them in place. No-op for
  // pure data. This is the single HOT entry, cached here; every COLD operation on the type
  // (place/erase/move/wire, and multi-arg operators like `+`) lives in the operator
  // REGISTRY, keyed by (name, endpoints), resolved at build time. See ../stage0.md.
  void (*tick)(syg_value_t self);
  // The type's source (declarative, not code): a graph's topology, native source text, or
  // {} (unspecified/native). What ships to a peer, hashes into identity, and reconstructs.
  syg_value_t impl;
};
}
