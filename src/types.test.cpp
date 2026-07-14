// types.test.cpp — decree-v1 constructors: identity falls out of content.
#include "types.hpp"
#include <cassert>
#include <cstring>
using namespace syg;

int main() {
  // fiat is deterministic and recomputable; roster rows are distinct.
  assert(ATOM.id == fiat("atom").id);
  assert(!(ATOM.id == STRUCTURE.id));

  // STRING is a minted atom (not a roster row): unsized, named by fiat("string").
  assert(STRING.type == ATOM.id);
  assert(((atom_term*)STRING.data)->size == DYNAMIC);
  assert(((atom_term*)STRING.data)->name == fiat("string").id);

  // atom: same ground facts => same id; different facts => different id.
  syg_hash f32name = string_node("float32").id;
  syg_handle_t f32 = atom(f32name, 4, 4);
  assert(f32.id == atom(f32name, 4, 4).id);
  assert(!(f32.id == atom(f32name, 8, 8).id));
  assert(((atom_term*)f32.data)->size == 4 && f32.size == sizeof(atom_term));

  // a zero-size atom is legal (type_tag for metaprogramming) and distinct.
  syg_handle_t tag = atom(string_node("type_tag").id, 0, 1);
  assert(!(tag.id == f32.id));

  // structure vs variant: SAME term bytes, different constructor => different id.
  field_term xy[] = { {string_node("x").id, f32.id}, {string_node("y").id, f32.id} };
  syg_handle_t s = structure(string_node("vec2").id, xy);
  syg_handle_t v = variant(string_node("vec2").id, xy);
  assert(s.size == v.size && std::memcmp(s.data, v.data, s.size) == 0);
  assert(!(s.id == v.id));

  // arity falls out of the handle, unstored.
  assert((s.size - sizeof(syg_hash)) / sizeof(field_term) == 2);

  // the name is identity: anonymous (GROUND) differs from named.
  assert(!(structure(GROUND, xy).id == s.id));

  // pointers: access flavors split the id; the term is just the pointee.
  assert(!(mutable_ptr(f32.id).id == constant_ptr(f32.id).id));
  assert(*(syg_hash*)constant_ptr(f32.id).data == f32.id);

  // ids verify: rehash the content, get the name back (the IPFS property).
  assert(syg_id(s) == s.id && syg_id(f32) == f32.id && syg_id(STRING) == STRING.id);
  return 0;
}
