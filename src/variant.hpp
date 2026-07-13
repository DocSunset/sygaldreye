#pragma once
// variant.hpp — the sum type as a runtime-minted syg_type (stage 0).
//
// A variant instance is { uint32 tag, inline data[N] }; its CASES are its template_args
// (all type-args). RAII dispatches on the tag through that table, so ONE set of
// functions serves every case-set — the primary producer is `make`, minting a descriptor
// at spawn. std::variant is NOT the wire form (its layout is impl-defined); author with
// an opaque leaf, reason about sums through this. Reasoning: ../stage0.md.
#include "stage0.hpp"
#include "syg.hpp"
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <span>

namespace syg::variant {

inline std::uint32_t& key(syg_value_t v) { return *static_cast<std::uint32_t*>(v.data); }
inline syg_value_t   blob(syg_value_t v) {
  return { v.type->template_args[key(v)].type,
           static_cast<char*>(v.data) + v.type->members[1].offset };
}

inline void place(syg_value_t v, void**) { key(v) = 0; auto c = blob(v); c.type->place(c, nullptr); }
inline void erase(syg_value_t v)         { auto c = blob(v); c.type->erase(c); }
inline void move(syg_value_t dst, void* src) {
  syg_value_t s{ dst.type, src };
  key(dst) = key(s);
  auto c = blob(dst);
  c.type->move(c, blob(s).data);
}
inline void set(syg_value_t v, std::uint32_t k) {          // the one verb products lack
  erase(v); key(v) = k;
  auto c = blob(v); c.type->place(c, nullptr);
}

// an opaque leaf: layout only (size/align), raw-byte RAII. The variant's `data` field
// points at one; its RAII is never called (the tag dispatches to the real case).
inline void raw_place(syg_value_t, void**) {}
inline void raw_erase(syg_value_t) {}
inline void raw_move(syg_value_t dst, void* src) { std::memcpy(dst.data, src, dst.type->size); }
inline const syg_type_t* opaque(std::size_t size, std::size_t align) {
  return new syg_type_t{ 0, 0, 0, 0, "opaque", nullptr, size, align,
                         0, nullptr, 0, nullptr, raw_place, raw_erase, raw_move };
}

inline syg_hash shape(std::span<const syg_type_t* const> cases) {
  syg_hash h = syg_hash_mix(sizeof(std::uint32_t), 3);   // seed with the tag's leaf atom
  for (auto* c : cases) h = syg_hash_mix(h, c->shape);
  return h;
}

inline const syg_type_t* make(const char* name, std::span<const syg_type_t* const> cases) {
  std::size_t N = 0, A = alignof(std::uint32_t);
  for (auto* c : cases) { N = std::max(N, c->size); A = std::max(A, c->align); }
  std::size_t data_off = (sizeof(std::uint32_t) + A - 1) / A * A;

  auto* args = new syg_template_arg_t[cases.size()];
  syg_hash params = 0;
  for (std::size_t i = 0; i < cases.size(); ++i) {
    args[i] = { cases[i]->name, cases[i], nullptr };
    params = syg_hash_mix(params, cases[i]->id);
  }
  auto* fields = new syg_field_t[2]{ { "tag", stage0::generate_value<std::uint32_t>(), 0 },
                                     { "data", opaque(N, A), data_off } };
  syg_hash nh = syg_hash_str(name);
  return new syg_type_t{
    syg_hash_mix(syg_hash_mix(0, nh), params), nh, 0, shape(cases),
    name, nullptr, data_off + N, A, 2, fields, cases.size(), args,
    place, erase, move,
  };
}

}  // namespace syg::variant
