// env.test.cpp — the environment: floor, organs, birth-registered nodes,
// method dispatch. Every node here is a store resident (no unregistered
// nodes besides the inscribed roster).
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

  // symbol vs string: same chars, different type => different id.
  syg_handle_t sx = symbol_node(env, "x");
  assert(!(sx.id == string_node(env, "x").id) && sx.type == SYMBOL.id);

  // STRING is an ordinary resident atom: unsized, named by a SYMBOL.
  syg_handle_t st = string_type(env);
  assert(st.type == ATOM.id && ((atom_term*)st.data)->size == DYNAMIC);

  // atoms: idempotent mint (same facts, same resident); name decodes HERE.
  syg_handle_t f32 = atom(env, symbol_node(env, "float32").id, 4, 4);
  assert(atom(env, symbol_node(env, "float32").id, 4, 4).data == f32.data);
  assert(!(atom(env, symbol_node(env, "float32").id, 8, 8).id == f32.id));
  const syg_handle_t* nm = get(env, ((atom_term*)f32.data)->name);
  assert(nm && nm->type == SYMBOL.id && std::memcmp(nm->data, "float32", 7) == 0);

  // values are content: every "4" dedupes to one node.
  assert(u64_node(env, 4).data == u64_node(env, 4).data);
  assert(!(u64_node(env, 4).id == u64_node(env, 5).id));

  // VERIFIED dedup: same id, different bytes => throw, never a silent merge.
  bool threw = false;
  try { insert_or_get(env, {.data = (void*)"evil!", .type = st.id, .id = sx.id,
                            .size = 5, .env = nullptr}); }
  catch (const std::exception&) { threw = true; }
  assert(threw);

  // structure vs variant: SAME term bytes, different constructor => different
  // id; arity falls out of the handle, unstored; anonymity is identity.
  field_term xy[] = { {symbol_node(env, "x").id, f32.id}, {symbol_node(env, "y").id, f32.id} };
  syg_handle_t s = structure(env, symbol_node(env, "vec2").id, xy);
  syg_handle_t v = variant(env, symbol_node(env, "vec2").id, xy);
  assert(s.size == v.size && std::memcmp(s.data, v.data, s.size) == 0 && !(s.id == v.id));
  assert((s.size - sizeof(syg_hash)) / sizeof(field_term) == 2);
  assert(!(structure(env, GROUND, xy).id == s.id));

  // pointers: access flavors split the id; the term is just the pointee.
  assert(!(mutable_ptr(env, f32.id).id == constant_ptr(env, f32.id).id));
  assert(*(syg_hash*)constant_ptr(env, f32.id).data == f32.id);

  // scope chains dedupe; qualification is identity.
  syg_handle_t geo = scope(env, GROUND, "geo");
  syg_handle_t v2  = scope(env, geo.id, "vec2");
  assert(((scope_term*)v2.data)->parent == geo.id);
  assert(scope(env, GROUND, "geo").data == geo.data);
  assert(!(structure(env, v2.id, xy).id == s.id));

  // environments compose: an organless child writes through to the floor;
  // a child with its OWN organ overlays (writes stay local, floor visible).
  syg_env_t child{env, {}};
  syg_handle_t f16 = atom(&child, symbol_node(&child, "float16").id, 2, 2);
  assert(get(env, f16.id));
  syg_env_t scratch{env, {}};
  content_store local;
  wire(&scratch, CONTENT.id, {.data = &local, .type = GROUND, .id = {},
                              .size = sizeof local, .env = nullptr});
  syg_handle_t f64 = atom(&scratch, symbol_node(&scratch, "float64").id, 8, 8);
  assert(get(&scratch, f64.id) && !get(env, f64.id) && get(&scratch, f32.id));

  // symbols: conferred HERE, invisible above, mortal. refs point at EITHER
  // world; rebind = a name changing its mind.
  static int cell = 42;
  syg_hash CELL = symbol_node(env, "cell").id;
  wire(&child, CELL, {.data = &cell, .type = GROUND, .id = {}, .size = sizeof cell, .env = nullptr});
  assert(find(&child, CELL) && *(int*)find(&child, CELL)->data == 42 && !find(env, CELL));
  syg_hash r = derived(SYMBOL.id, f32.id);
  bind(&child, r, {address::symbol, CELL});
  assert(*(int*)follow(&child, r)->data == 42);
  bind(&child, r, {address::content, f32.id});
  assert(follow(&child, r)->id == f32.id);
  unwire(&child, CELL);
  assert(!find(&child, CELL));

  // emplace_or_get: mint BY TYPE ID — dispatch and the typed path agree.
  // The marshaller reads the method's signature: name by reference (&arg.id),
  // sizes through payloads; structure's count endpoint arrives as a u64 node.
  syg_handle_t args[] = { symbol_node(env, "float16"), u64_node(env, 2), u64_node(env, 2) };
  syg_handle_t e16 = emplace_or_get(env, ATOM.id, 3, args);
  assert(e16.id == f16.id && e16.data == get(env, f16.id)->data);
  syg_handle_t sargs[] = { u64_node(env, 2), symbol_node(env, "vec2"),
                           symbol_node(env, "x"), f32, symbol_node(env, "y"), f32 };
  assert(emplace_or_get(env, STRUCTURE.id, 6, sargs).id == s.id);

  // the method's type IS its signature — a resident, decodable structure.
  const syg_handle_t* m = follow(env, derived(CONSTRUCT.id, ATOM.id));
  assert(m && get(env, m->type) && get(env, m->type)->type == STRUCTURE.id);

  // dispatch is all-purpose: any relation, any subject — a user relation is
  // just a minted symbol bound to a method like any other.
  method rm = resolve(env, CONSTRUCT.id, ATOM.id);
  assert(rm.fn && rm.sig);                       // the cold half, cacheable
  assert(call(env, rm, 3, args).id == f16.id);   // the hot half, no lookups
  assert(dispatch(env, CONSTRUCT.id, ATOM.id, 3, args).id == f16.id);
  return 0;
}
