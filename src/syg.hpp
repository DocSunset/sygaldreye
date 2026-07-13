#pragma once
// syg.hpp — the type-erased type ABI (stage 0).
//
// POD, C-ABI structs a foreign .so could hand back: a type descriptor (identity +
// layout + RAII). This is the shape the reflection layer (stage0.hpp) generates into
// and the runtime type-family builders (syg::variant, …) mint at spawn. The reasoning
// lives in ../stage0.md.
#include <cstddef>
#include <cstdint>

// ── identity hashes: FNV-1a, 64-bit. A DEFINED function (NOT std::hash, which isn't
//    stable across implementations) so every peer computes the same value. Pinned —
//    a peer that hashes differently can't agree on a type. For runtime identity
//    compares only, not content-addressing (widen `shape` to crypto if that changes).
using syg_hash = std::uint64_t;
// runtime blob hash — for value bytes/params. The void* pun bars it from constant
// expressions, so the compile-time paths below never route through it.
inline syg_hash syg_hash_bytes(const void* p, std::size_t n, syg_hash h = 0xcbf29ce484222325ull) {
  const unsigned char* b = static_cast<const unsigned char*>(p);
  for (std::size_t i = 0; i < n; ++i) { h ^= b[i]; h *= 0x100000001b3ull; }
  return h;
}
// constexpr-clean twins (no punning): a name over its chars, a mix that absorbs the
// eight bytes of `b` LSB-first (endian-defined ⇒ same value on every peer).
constexpr syg_hash syg_hash_str(const char* s, syg_hash h = 0xcbf29ce484222325ull) {
  for (; *s; ++s) { h ^= static_cast<unsigned char>(*s); h *= 0x100000001b3ull; }
  return h;
}
constexpr syg_hash syg_hash_mix(syg_hash a, syg_hash b) {
  for (int i = 0; i < 8; ++i) { a ^= (b >> (i * 8)) & 0xff; a *= 0x100000001b3ull; }
  return a;
}

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
  // RAII over a typed object. `self`/`dst` bundle (type, storage) as a syg_value_t so a
  // runtime-generic impl (a variant, any minted type family) can dispatch on its own type.
  void (*place)(syg_value_t self, void** inputs);  // ctor + bind each const T* input to inputs[i]
  void (*erase)(syg_value_t self);                 // dtor, no free
  void (*move)(syg_value_t dst, void* src);        // migrate from same-typed src: steal owned, copy borrowed
};
}
