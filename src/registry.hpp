#pragma once
// registry.hpp — the environment: this peer's table of nodes. The map layout
// is peer business (only behavior is contract); environments COMPOSE — a miss
// asks the parent, so "my overlay over your base over the fiat floor" is one
// pointer chase. Content-addressed half only; the ref half (names, bindings,
// subscriptions — book ch. 2) comes later. The registry is the first LOCAL
// node: its content mutates, so it has no content id — its identity is a ref
// question, deferred; for now it exists as the payload behind env pointers.
#include "types.hpp"
#include <unordered_map>

struct syg_registry_t {
  struct id_hash {  // an id IS a hash — hash it with a straight face
    std::size_t operator()(syg_hash h) const { return h.digest; }
  };
  syg_registry_t* parent;  // nullptr at the floor
  std::unordered_map<syg_hash, syg_handle_t, id_hash> table;  // id → grip; owns each data
};

namespace syg {

// resolve: walk HERE outward. nullptr = a miss everywhere — content this
// peer has never seen.
inline const syg_handle_t* get(const syg_registry_t* env, syg_hash id) {
  if (auto it = env->table.find(id); it != env->table.end()) return &it->second;
  return env->parent ? get(env->parent, id) : nullptr;
}

// insert_or_get: content-addressed ⇒ confluent (grow-only; a duplicate is a
// no-op returning the resident). The table COPIES the payload into storage it
// owns and stamps env; the caller keeps ownership of whatever it passed.
inline syg_handle_t insert_or_get(syg_registry_t* env, const syg_handle_t& h) {
  if (const syg_handle_t* found = get(env, h.id)) return *found;
  void* copy = std::malloc(h.size);
  std::memcpy(copy, h.data, h.size);
  return env->table.emplace(h.id, syg_handle_t{h.id, h.type, copy, h.size, env}).first->second;
}

// mint-through: register a freshly built term handle and free the temporary
// (types.hpp terms are malloc'd; the registered copy is the durable one).
inline syg_handle_t mint(syg_registry_t* env, syg_handle_t h) {
  syg_handle_t r = insert_or_get(env, h);
  std::free(h.data);
  return r;
}

// the authored conveniences — mint AND register the name, so every hash the
// term carries decodes here (the char* overload's whole reason to exist).
inline syg_handle_t atom(syg_registry_t* env, const char* name,
                         std::uint64_t size, std::uint64_t align) {
  return mint(env, atom(insert_or_get(env, string_node(name)).id, size, align));
}
inline syg_handle_t scope(syg_registry_t* env, syg_hash parent, const char* name) {
  return mint(env, scope(parent, insert_or_get(env, string_node(name)).id));
}

// the ground environment: a fresh floor with the decree pre-registered —
// every environment should chain down to one. This loop is the boot tape's
// first stanza in C++ costume.
inline syg_registry_t* floor() {
  auto* env = new syg_registry_t{nullptr, {}};
  for (const syg_handle_t& h : {ATOM, STRUCTURE, VARIANT, MUTABLE_PTR, CONSTANT_PTR, SCOPE,
                                fiat("string"), STRING})
    insert_or_get(env, h);
  return env;
}

}  // namespace syg
