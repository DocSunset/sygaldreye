// env.test.cpp — the environment: floor, organs, three contracts, composition.
#include "env.hpp"
#include <cassert>
#include <cstring>
using namespace syg;

int main() {
  syg_env_t* env = floor();

  // the decree is pre-registered and resolvable through the CONTENT organ.
  assert(get(env, content_id{ATOM.id}) && get(env, content_id{STRING.id}));

  // every hash the floor carries decodes: STRING's name is the fiat "string".
  const syg_handle_t* sname = get(env, content_id{((atom_term*)get(env, content_id{STRING.id})->data)->name});
  assert(sname && sname->type == GROUND && std::memcmp(sname->data, "string", 6) == 0);

  // insert copies (store owns), stamps the organ's frame; duplicate is a no-op.
  syg_handle_t probe = string_node("hello");
  syg_handle_t a = insert_or_get(env, probe);
  assert(a.data != probe.data && a.env == env && a.id == probe.id);
  assert(insert_or_get(env, string_node("hello")).data == a.data);

  // VERIFIED dedup: same id, different bytes => throw, never a silent merge.
  bool threw = false;
  try { insert_or_get(env, {a.id, STRING.id, (void*)"evil!", 5, nullptr}); }
  catch (const std::exception&) { threw = true; }
  assert(threw);

  // environments compose: a child frame with NO organs writes through to the
  // floor's store and resolves through it.
  syg_env_t child{env, {}};
  assert(get(&child, content_id{a.id}));
  syg_handle_t f32 = atom(&child, "float32", 4, 4);
  assert(get(env, content_id{f32.id}));  // landed in the floor's organ

  // a child with its OWN content organ overlays: writes stay local.
  syg_env_t scratch{env, {}};
  content_store local;
  wire(&scratch, CONTENT, { {}, GROUND, &local, sizeof local, nullptr });
  syg_handle_t f64 = atom(&scratch, "float64", 8, 8);
  assert(get(&scratch, content_id{f64.id}) && !get(env, content_id{f64.id}));
  assert(get(&scratch, content_id{f32.id}));  // floor content still visible

  // symbols: wire/find/unwire — conferred HERE, invisible above, mortal.
  static int cell = 42;
  sym_name CELL{fiat("cell").id};
  wire(&child, CELL, { {}, GROUND, &cell, sizeof cell, nullptr });
  const syg_handle_t* grip = find(&child, CELL);
  assert(grip && *(int*)grip->data == 42 && grip->id == CELL.h && !find(env, CELL));
  // refs: bind to a SYMBOL address (a live grip), then change the name's mind
  // to a CONTENT address — follow resolves into either world.
  ref_name r{derived(fiat("tick").id, f32.id)};
  bind(&child, r, {address::symbol, CELL.h});
  assert(follow(&child, r) && *(int*)follow(&child, r)->data == 42);
  bind(&child, r, {address::content, f32.id});
  assert(follow(&child, r) && follow(&child, r)->id == f32.id);
  unwire(&child, CELL);
  assert(!find(&child, CELL));

  // scope chains still mint and dedupe; qualification is identity.
  syg_handle_t geo = scope(&child, GROUND, "geo");
  syg_handle_t v2  = scope(&child, geo.id, "vec2");
  assert(((scope_term*)v2.data)->parent == geo.id);
  assert(scope(&child, GROUND, "geo").data == geo.data);
  field_term xy[] = { {string_node("x").id, f32.id}, {string_node("y").id, f32.id} };
  assert(!(structure(v2.id, xy).id == structure(string_node("vec2").id, xy).id));
  return 0;
}
