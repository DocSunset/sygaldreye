// registry.test.cpp — the environment: floor, confluence, composition, scopes.
#include "registry.hpp"
#include <cassert>
#include <cstring>
using namespace syg;

int main() {
  syg_registry_t* env = floor();

  // the decree is pre-registered and resolvable.
  assert(get(env, ATOM.id) && get(env, STRING.id) && get(env, SCOPE.id));

  // every hash the floor carries decodes: STRING's name is the fiat "string".
  const syg_handle_t* sname = get(env, ((atom_term*)get(env, STRING.id)->data)->name);
  assert(sname && sname->type == GROUND && std::memcmp(sname->data, "string", 6) == 0);

  // insert copies and stamps env; a duplicate is a no-op returning the resident.
  syg_handle_t probe = string_node("hello");
  syg_handle_t a = insert_or_get(env, probe);
  assert(a.data != probe.data && a.env == env && a.id == probe.id);
  assert(insert_or_get(env, string_node("hello")).data == a.data);

  // environments compose: a child resolves through its parent, never the reverse.
  syg_registry_t child{env, {}};
  assert(get(&child, a.id));
  syg_handle_t f32 = atom(&child, "float32", 4, 4);
  assert(get(&child, f32.id) && !get(env, f32.id));

  // the convenience registered the name too — the term decodes HERE.
  const syg_handle_t* nm = get(&child, ((atom_term*)f32.data)->name);
  assert(nm && nm->type == STRING.id && std::memcmp(nm->data, "float32", 7) == 0);

  // scope chains: geo::vec2; a shared prefix dedupes to the same resident.
  syg_handle_t geo = scope(&child, GROUND, "geo");
  syg_handle_t v2  = scope(&child, geo.id, "vec2");
  assert(((scope_term*)v2.data)->parent == geo.id);
  assert(scope(&child, GROUND, "geo").data == geo.data);

  // qualification is identity: geo::vec2 names a different type than ::vec2.
  field_term xy[] = { {string_node("x").id, f32.id}, {string_node("y").id, f32.id} };
  assert(!(structure(v2.id, xy).id == structure(string_node("vec2").id, xy).id));
  return 0;
}
