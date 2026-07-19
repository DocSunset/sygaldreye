// env.test.cpp — the environment: ONE table, grips and references; floor,
// birth-registered nodes, method dispatch. No unregistered nodes besides the
// inscribed roster.
#include "env.hpp"
#include <cassert>
#include <cstring>
using namespace syg;

int main() {
  syg_env_t* env = floor();

  // the decree is resident and every row rehashes to its id.
  for (const syg_handle_t& h : ROSTER) {
    const syg_handle_t* r = get(env, h.id);
    assert(r && r->data != h.data && syg_id(*r) == r->id && r->env == env);
  }

  // a name IS a string: one node, one id; the constexpr twin agrees.
  syg_handle_t sx = string_node(env, "x");
  assert(sx.type == STR_TYPE && string_type(env).id == STR_TYPE);
  static_assert(name_id("x").digest != 0);
  assert(name_id("x") == sx.id);

  // REF: the computed id and the minted resident agree.
  assert(ref_type(env).id == ref_type_id());

  // the canon is resident: every row minted at the floor, terms as decreed.
  for (const canon_row& r : CANON) {
    syg_handle_t t = canon_type(env, r.name);
    assert(get(env, t.id) && syg_id(t) == t.id);
    assert(((atom_term*)t.data)->name == r.term().name);  // str: fiat; rest: name_id
  }
  assert(canon_type<float>(env).id == canon_type(env, "f32").id);  // the kind-mapper's entry
  assert(get(env, CONSTRUCT));                                     // relations decode too

  // atoms: idempotent mint; the term's name decodes HERE.
  syg_handle_t f32 = atom(env, string_node(env, "float32").id, 4, 4);
  assert(atom(env, string_node(env, "float32").id, 4, 4).data == f32.data);
  assert(!(atom(env, string_node(env, "float32").id, 8, 8).id == f32.id));
  const syg_handle_t* nm = get(env, ((atom_term*)f32.data)->name);
  assert(nm && nm->type == STR_TYPE && std::memcmp(nm->data, "float32", 7) == 0);

  // values are content: every "4" dedupes to one node.
  assert(u64_node(env, 4).data == u64_node(env, 4).data);

  // VERIFIED dedup: same id, different bytes => throw, never a silent merge.
  bool threw = false;
  try { insert_or_get(env, {.data = (void*)"evil!", .type = STR_TYPE, .id = sx.id,
                            .size = 5, .env = nullptr}); }
  catch (const std::exception&) { threw = true; }
  assert(threw);

  // structure vs variant: SAME term bytes, different constructor => different
  // id; arity falls out of the handle; anonymity is identity.
  field_term xy[] = { {string_node(env, "x").id, f32.id}, {string_node(env, "y").id, f32.id} };
  syg_handle_t s = structure(env, string_node(env, "vec2").id, xy);
  syg_handle_t v = variant(env, string_node(env, "vec2").id, xy);
  assert(s.size == v.size && std::memcmp(s.data, v.data, s.size) == 0 && !(s.id == v.id));
  assert((s.size - sizeof(syg_hash)) / sizeof(field_term) == 2);
  assert(!(structure(env, GROUND, xy).id == s.id));

  // pointers; scope chains dedupe; qualification is identity.
  assert(!(mutable_ptr(env, f32.id).id == constant_ptr(env, f32.id).id));
  syg_handle_t geo = scope(env, GROUND, "geo");
  syg_handle_t v2  = scope(env, geo.id, "vec2");
  assert(((scope_term*)v2.data)->parent == geo.id);
  assert(!(structure(env, v2.id, xy).id == s.id));

  // environments compose: an organless child writes through to the floor;
  // a child with its OWN organ overlays (writes stay local, floor visible).
  syg_env_t child{env, {}};
  syg_handle_t f16 = atom(&child, string_node(&child, "float16").id, 2, 2);
  assert(get(env, f16.id));
  syg_env_t scratch{env, {}};
  content_store local;
  wire(&scratch, GROUND, CONTENT.id, {.data = &local, .type = GROUND, .id = {},
                                      .size = sizeof local, .env = nullptr});
  syg_handle_t f64 = atom(&scratch, string_node(&scratch, "float64").id, 8, 8);
  assert(get(&scratch, f64.id) && !get(env, f64.id) && get(&scratch, f32.id));

  // ── the ONE table: grips and references ──
  // a GRIP: conferred HERE, invisible above, mortal; id = {} (refers to nothing).
  static int cell = 42;
  syg_hash CELL = string_node(env, "cell").id;
  wire(&child, GROUND, CELL, {.data = &cell, .type = GROUND, .id = {}, .size = sizeof cell,
                              .env = nullptr});
  assert(find(&child, GROUND, CELL) && *(int*)find(&child, GROUND, CELL)->data == 42);
  assert(find(&child, GROUND, CELL)->id == syg_hash{} && !find(env, GROUND, CELL));

  // a REFERENCE: bind, and find derefs into the content store — a real
  // handle back, always. peek sees the raw row: type REF, no bytes, the
  // target riding in .id. Rebind = the name changing its mind.
  bind(&child, GROUND, sx.id, f32.id);
  assert(find(&child, GROUND, sx.id)->id == f32.id);
  assert(find(&child, GROUND, sx.id)->data == get(env, f32.id)->data);   // deref'd
  const syg_handle_t* raw = peek(&child, GROUND, sx.id);
  assert(raw && raw->type == ref_type_id() && raw->data == nullptr && raw->id == f32.id);
  bind(&child, GROUND, sx.id, s.id);
  assert(find(&child, GROUND, sx.id)->id == s.id);
  unwire(&child, GROUND, CELL);
  assert(!find(&child, GROUND, CELL));

  // emplace_or_get: mint BY TYPE ID — dispatch and the typed path agree.
  syg_handle_t args[] = { string_node(env, "float16"), u64_node(env, 2), u64_node(env, 2) };
  syg_handle_t e16 = emplace_or_get(env, ATOM.id, 3, args);
  assert(e16.id == f16.id && e16.data == get(env, f16.id)->data);
  syg_handle_t sargs[] = { string_node(env, "vec2"),
                           string_node(env, "x"), f32, string_node(env, "y"), f32 };
  assert(emplace_or_get(env, STRUCTURE.id, 5, sargs).id == s.id);

  // the method's type IS its signature; binding is ONE row of the ONE table.
  method rm = resolve(env, CONSTRUCT, ATOM.id);
  assert(rm.fn && rm.sig && rm.sig->type == STRUCTURE.id);
  assert(call(env, rm, 3, args).id == f16.id);
  return 0;
}
