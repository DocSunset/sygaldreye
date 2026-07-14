#pragma once
#include "syg.hpp"
#include <array>
#include <cstdlib>
#include <meta>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

namespace syg::stage0 {

// A word is the executable form of a node, named in homage to forth. State is
// opaque bytes; the word is the ONLY place that knows the types. It gets ONE array
// of cell pointers — the first in_count are inputs, the rest outputs (inputs then
// outputs, like a component's members) — and reads/writes through them.
using word = void (*)(void** slots);

struct binding;   // the executable role; defined before make_binding, which lays one out

// A node is a declaration: a word plus what the planner needs to lay out state —
// the sizes of each input and output cell (the counts are the spans' lengths). This
// struct is ONE reification of a node in its declaration role; `binding`
// reifies the executable binding role.
struct node {
  std::span<const std::size_t>      in_sizes;
  std::span<const std::size_t>      out_sizes;
  std::span<const std::string_view> in_names;    // one per input, declaration order
  std::span<const std::string_view> out_names;   // one per output (today just "out")
  std::string_view                  name;        // the node's own name — the function's
  std::span<const std::string_view> namespaces;  // enclosing namespaces, outermost first
  word                              fn;
};

// enclosing_namespaces reads a type-or-function's namespace chain off its ENTITY.
// The caller freezes the result into static storage (identifier_of's chars are
// already static, so the string_views stay valid at runtime).
consteval std::vector<std::string_view> enclosing_namespaces(std::meta::info x) {
  namespace m = std::meta;
  std::vector<std::string_view> v;
  if (!m::has_parent(x)) return v;               // a primitive (e.g. ^^float) has no parent
  for (m::info s = m::parent_of(x); m::is_namespace(s) && m::has_identifier(s); s = m::parent_of(s))
    v.insert(v.begin(), m::identifier_of(s));    // walk up; outermost ends up first
  return v;
}

// A return type annotated [[=outputs{}]] is not one output cell but several: its
// members become the node's outputs. Same C++ shape as a single-cell struct like
// vec3 — the annotation is the intent that says "splay me", which no struct shape
// could tell us on its own.
struct outputs {};

consteval bool splayed(std::meta::info R) {
  for (std::meta::info a : std::meta::annotations_of(R))       // annotation values are const,
    if (std::meta::remove_const(std::meta::type_of(a)) == ^^outputs) return true;   // so strip it
  return false;
}
consteval std::vector<std::meta::info> out_members_of(std::meta::info R) {
  namespace m = std::meta;
  if (m::is_void_type(R) || !splayed(R)) return {};        // void or single: no members
  return m::nonstatic_data_members_of(R, m::access_context::current());
}
// define_static_array can't persist std::string_view (non-structural — private
// members), so freeze a name list into a static std::array by value. The chars are
// already static (identifier_of's guarantee); only the view objects need a home.
template <std::vector<std::string_view> (*Gen)(std::meta::info), std::meta::info X>
consteval auto frozen_names() {
  constexpr std::size_t n = Gen(X).size();
  std::array<std::string_view, n> a{};
  std::vector<std::string_view> v = Gen(X);
  for (std::size_t i = 0; i < n; ++i) a[i] = v[i];
  return a;
}

// ── the type-rich twin: component<^^fn> and describe_component<C> ──────────────
// component<^^fn> generates a struct shaped like an authored native node: a const
// T& per parameter (named after it) referencing an upstream output, an OWNED T out,
// and an operator() that runs the function. describe_component<C> is the reverse —
// it erases an authored component type back into a type-erased node. REQUIRES
// -freflection.
//
// Three rules the compiler enforces, encoded below:
//   1. define_aggregate must run from a `consteval { }` BLOCK, never a function.
//   2. that block must share the target type's scope — hence `data` is NESTED.
//   3. splicing a std::meta::info in a runtime body escalates it to consteval —
//      so Fn becomes a function pointer once, and member infos ride as template
//      arguments (a constant context), never as runtime values.
// the output members a faithful component needs: none for void, one per member for
// a [[=outputs{}]] return (splayed, named after each), else one owned "out".
consteval std::vector<std::meta::info> output_member_specs(std::meta::info Fn) {
  namespace m = std::meta;
  m::info R = m::return_type_of(Fn);
  std::vector<m::info> v;
  if (m::is_void_type(R)) return v;
  if (splayed(R))
    for (m::info x : out_members_of(R))
      v.push_back(m::data_member_spec(m::type_of(x), {.name = m::identifier_of(x)}));
  else
    v.push_back(m::data_member_spec(R, {.name = "out"}));
  return v;
}

template <std::meta::info Fn>
struct component_generator {
  // Same input-purity rule as describe_function: a node must not silently mutate
  // its inputs. Value and const T& are fine; a mutable or rvalue reference is not.
  static consteval bool inputs_pure() {
    namespace m = std::meta;
    for (m::info p : m::parameters_of(Fn)) {
      m::info t = m::type_of(p);
      if (m::is_rvalue_reference_type(t)) return false;
      if (m::is_lvalue_reference_type(t) && !m::is_const_type(m::remove_reference(t))) return false;
    }
    return true;
  }
  static_assert(inputs_pure(),
      "a component input must not be a mutable or rvalue reference — take it by value or const T&");

  struct data;                                   // nested & incomplete (rule 2)
  consteval {                                     // a BLOCK (rule 1)
    namespace m = std::meta;
    std::vector<m::info> mem;
    for (m::info p : m::parameters_of(Fn))       // one const T& per parameter, named
      mem.push_back(m::data_member_spec(
          m::add_lvalue_reference(m::add_const(m::type_of(p))), {.name = m::identifier_of(p)}));
    for (m::info s : output_member_specs(Fn)) mem.push_back(s);   // 0, 1, or N owned outputs
    m::define_aggregate(^^data, mem);
  }

  static constexpr auto fn = &[: Fn :];           // Fn spliced once -> runtime fn pointer (rule 3)

  struct generated_component : data {             // inherit the members: access is A.a, not A.d.a
    static consteval std::meta::info origin() { return Fn; }   // the function this component IS —
                                    // preserves identity (name, namespaces) across the erasure
    void operator()() {                           // run fn(inputs) -> owned outputs
      namespace m = std::meta;
      static constexpr auto mem = std::define_static_array(
          m::nonstatic_data_members_of(^^data, m::access_context::current()));
      constexpr std::size_t nin = m::parameters_of(Fn).size();   // inputs come first, then outputs
      constexpr m::info R = m::return_type_of(Fn);
      constexpr bool is_void  = m::is_void_type(R);              // bind consteval results before the
      constexpr bool is_splay = splayed(R);                      // if constexpr (GCC 16 mis-selects)
      auto call = [this]<std::size_t... I>(std::index_sequence<I...>) {
        return fn(this->[: mem[I] :]...);         // splice static-constexpr infos (rule 3)
      };
      if constexpr (is_void)
        call(std::make_index_sequence<nin>{});
      else if constexpr (!is_splay)
        this->[: mem[nin] :] = call(std::make_index_sequence<nin>{});   // the lone owned out
      else {                                      // splay the returned struct across owned members
        auto tmp = call(std::make_index_sequence<nin>{});
        static constexpr auto rm = std::define_static_array(out_members_of(R));
        [this, &tmp]<std::size_t... J>(std::index_sequence<J...>) {
          ((this->[: mem[nin + J] :] = tmp.[: rm[J] :]), ...);
        }(std::make_index_sequence<mem.size() - nin>{});
      }
    }
  };
};

// describe_component: the reverse of component<> — erase an authored component type
// into a type-erased node. Reference members are inputs (referenced upstream), value
// members are owned outputs (inputs must precede outputs in declaration order). The
// shim constructs C over the input cells, ticks it, and writes each owned output to
// its cell — the copy elides (C never escapes).
consteval std::vector<std::meta::info> comp_members(std::meta::info C, bool refs) {
  namespace m = std::meta;
  std::vector<m::info> v;
  for (m::info x : m::nonstatic_data_members_of(C, m::access_context::current()))
    if (m::is_lvalue_reference_type(m::type_of(x)) == refs) v.push_back(x);
  return v;
}
consteval std::vector<std::size_t> comp_sizes(std::meta::info C, bool refs) {
  namespace m = std::meta;
  std::vector<std::size_t> v;
  for (m::info x : comp_members(C, refs)) {
    m::info t = m::type_of(x);
    v.push_back(m::size_of(refs ? m::remove_reference(t) : t));
  }
  return v;
}
// member names, split the same way as sizes. Single-arg wrappers so frozen_names
// (which persists string_views past the structural-type limit) can freeze them.
consteval std::vector<std::string_view> comp_names(std::meta::info C, bool refs) {
  std::vector<std::string_view> v;
  for (std::meta::info x : comp_members(C, refs)) v.push_back(std::meta::identifier_of(x));
  return v;
}
consteval std::vector<std::string_view> comp_in_names(std::meta::info C)  { return comp_names(C, true); }
consteval std::vector<std::string_view> comp_out_names(std::meta::info C) { return comp_names(C, false); }

// where a component's members actually live: its own body if authored, or its `data`
// base if generated by component<> (which parks members in a base to leave room for
// operator()). nonstatic_data_members_of doesn't cross bases, so reflect them THERE.
consteval std::meta::info data_type(std::meta::info C) {
  namespace m = std::meta;
  auto bs = m::bases_of(C, m::access_context::current());
  return bs.empty() ? C : m::type_of(bs[0]);
}

// a generated component reports the function it came from; an authored one doesn't,
// so its identity falls back to the type's own name and namespaces.
template <class C> concept has_origin = requires { C::origin(); };

template <typename C>
node describe_component() {
  namespace m = std::meta;
  static constexpr m::info D = data_type(^^C);   // members live here: C, or its data base
  static constexpr auto ins   = std::define_static_array(comp_members(D, true));
  static constexpr auto outs  = std::define_static_array(comp_members(D, false));
  static constexpr auto insz  = std::define_static_array(comp_sizes(D, true));
  static constexpr auto outsz = std::define_static_array(comp_sizes(D, false));
  static constexpr auto innm  = frozen_names<comp_in_names, D>();
  static constexpr auto outnm = frozen_names<comp_out_names, D>();
  static constexpr auto spaces = [] {            // identity: prefer the origin function's
    if constexpr (has_origin<C>) return frozen_names<enclosing_namespaces, C::origin()>();
    else                         return frozen_names<enclosing_namespaces, ^^C>();
  }();
  static constexpr std::string_view self = [] {
    if constexpr (has_origin<C>) return std::string_view{m::identifier_of(C::origin())};
    else                         return std::string_view{m::identifier_of(^^C)};
  }();
  return {
    .in_sizes = insz, .out_sizes = outsz,        // members split by ref/value, named after
    .in_names = innm, .out_names = outnm,        // themselves; the node's own name is the type's
    .name = self, .namespaces = spaces,
    .fn =
    [](void** slots) {
      constexpr std::size_t nin = ins.size();            // outputs start at slots[in_count]
      // a generated component keeps its members in a `data` base, so its init needs
      // an extra brace level; an authored one has them direct. based picks the form.
      constexpr bool based = D != ^^C;   // generated components park members in a base
      C c = [&]<std::size_t... I, std::size_t... J>(std::index_sequence<I...>, std::index_sequence<J...>) {
        if constexpr (based)
          return C{ { *static_cast<typename [: m::remove_reference(m::type_of(ins[I])) :]*>(slots[I])...,
                      (typename [: m::type_of(outs[J]) :]){}... } };
        else
          return C{ *static_cast<typename [: m::remove_reference(m::type_of(ins[I])) :]*>(slots[I])...,
                    (typename [: m::type_of(outs[J]) :]){}... };
      }(std::make_index_sequence<ins.size()>{}, std::make_index_sequence<outs.size()>{});
      c();
      [&]<std::size_t... J>(std::index_sequence<J...>) {
        ( (*static_cast<typename [: m::type_of(outs[J]) :]*>(slots[nin + J]) = c.[: outs[J] :]), ... );
      }(std::make_index_sequence<outs.size()>{});
    }
  };
}

template <std::meta::info Fn>
using component = typename component_generator<Fn>::generated_component;

// a free function IS a component (params in, return out). So describing one is just:
// generate its faithful component, then erase that to a node. The component carries
// void/splay outputs and the function's identity (origin), so nothing is lost.
template <std::meta::info Fn> node describe_function() { return describe_component<component<Fn>>(); }

// A binding is one blob: [ fn ][ slots ][ outmem ]. The slots array (first
// in_count are inputs, the rest outputs) lives INLINE right after fn, and the
// output slots point into outmem — the owned output cells, later in the same blob.
struct binding {
  word fn;
  // [slots], if any
  // [outputs memory], if any
  void operator()() const { fn((void**)(this+1)); }
};

// make_binding: a self-owning binding — one heap blob, [fn][slots][outmem]. Inputs
// start unwired (nullptr); each output points at an owned cell just past the slot
// array. The binding owns its output the way a component owns `T out`; the only
// difference is placement, forced by erasure — the size isn't known until runtime.
// (Outputs pack flat after the 8-aligned slot array: fine for cell-shaped cells.)
binding* make_binding(const node& n) {
  const std::size_t nin = n.in_sizes.size();
  const std::size_t nout = n.out_sizes.size();
  const std::size_t nendpoints = nin + nout;

  std::size_t outbytes = 0;
  for (std::size_t sz : n.out_sizes) outbytes += sz;

  std::size_t bindingbytes = sizeof(binding) + (nendpoints) * sizeof(void*) + outbytes;
  binding* b = (binding*)std::malloc(bindingbytes);

  b->fn = n.fn;
  void** s = (void**)++b;
  unsigned char* out = (unsigned char*)(s + nin + nout);
  for (std::size_t j = 0; j < nin;  ++j) s[j] = nullptr; // inputs unwired
  for (std::size_t j = 0; j < nout; ++j) { s[nin + j] = out; out += n.out_sizes[j]; }
  return b;
}

// ── reflection → syg_type_t: mint the POD type descriptor for a data type ───────────
// shared naming/identity plumbing. A user type carries an identifier; a primitive is a
// keyword (no identifier), so fall back to its display name. scope_fold folds the
// enclosing namespaces; leaf_category is the byte-semantics tag that keeps float(4,fp)
// distinct from uint32(4,uint) — 0 means "opaque" (a non-arithmetic held as a leaf).
template <class T> consteval const char* type_name() {
  namespace m = std::meta;
  if constexpr (m::has_identifier(^^T)) return std::define_static_string(m::identifier_of(^^T));
  else                                  return std::define_static_string(m::display_string_of(^^T));
}
template <class T> consteval syg_hash scope_fold() {
  syg_hash h{};
  for (std::string_view s : enclosing_namespaces(^^T)) h = syg_hash::mix(h, syg_hash::str(std::define_static_string(s)));
  return h;
}
template <class T> consteval std::uint64_t leaf_category() {   // a code, not a digest
  if constexpr (std::is_floating_point_v<T>) return 1;
  else if constexpr (std::is_signed_v<T>)    return 2;
  else if constexpr (std::is_unsigned_v<T>)  return 3;
  else return 0;
}

// generate_value<T>: the BASE CASE — a leaf syg_type. No fields; T IS the value; RAII
// straight from T's own special members (a float, or an opaque type held whole). shape
// is the byte-semantics atom, never a field fold, because a leaf has no fields to fold.
template <class T> inline constexpr syg_type_t value_type_v = {
  /*id*/ syg_hash::mix(scope_fold<T>(), syg_hash::str(type_name<T>())),   // params_hash: a value has no statics
  /*name_hash*/ syg_hash::str(type_name<T>()), /*scope_hash*/ scope_fold<T>(),
  /*shape*/ syg_hash::mix({sizeof(T)}, leaf_category<T>()),
  /*name*/ type_name<T>(), /*scope*/ nullptr, /*size*/ sizeof(T), /*align*/ alignof(T),
  /*members*/ 0, nullptr, /*template args*/ 0, nullptr,
  /*tick*/  [](syg_value_t){},                                  // pure data: no behavior
  /*impl*/  {},                                                 // native (source unspecified)
};
template <class T> consteval const syg_type_t* generate_value() { return &value_type_v<T>; }

// generate_component<T>: THE BRANCH + THE RECURSION. Arithmetic ⇒ leaf (generate_value);
// else reflect fields and recurse each field type through the same branch. Grounds out
// at leaves. (A leaf-set policy — treat chosen templates like std::vector<X> as opaque
// leaves — is the planned extension; today only arithmetic terminates the descent.)
template <class T> consteval const syg_type_t* generate_component();

// member infos ride as a static array indexed by a pack, so each field's type splices
// in a CONSTANT context (a runtime loop variable can't be spliced — see operator()).
template <class T> inline constexpr auto member_infos =
    std::define_static_array(std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::current()));
template <class T, std::size_t... I>
consteval auto build_fields(std::index_sequence<I...>) {
  namespace m = std::meta;
  return std::array<syg_field_t, sizeof...(I)>{ syg_field_t{
      std::define_static_string(m::identifier_of(member_infos<T>[I])),
      generate_component<typename [: m::type_of(member_infos<T>[I]) :]>(),   // recurse
      m::offset_of(member_infos<T>[I]).bytes }... };
}
template <class T> inline constexpr auto fields_v =
    build_fields<T>(std::make_index_sequence<member_infos<T>.size()>{});

template <class T> consteval syg_hash product_shape() {          // fold of the field shapes
  syg_hash h{};
  for (const syg_field_t& f : fields_v<T>) h = syg_hash::mix(h, f.type->shape);
  return h;
}
template <class T> inline constexpr syg_type_t component_type_v = {
  /*id*/ syg_hash::mix(scope_fold<T>(), syg_hash::str(type_name<T>())),
  /*name_hash*/ syg_hash::str(type_name<T>()), /*scope_hash*/ scope_fold<T>(),
  /*shape*/ product_shape<T>(),
  /*name*/ type_name<T>(), /*scope*/ nullptr, /*size*/ sizeof(T), /*align*/ alignof(T),
  /*members*/ fields_v<T>.size(), fields_v<T>.data(), /*template args*/ 0, nullptr,
  /*tick*/  [](syg_value_t){},
  /*impl*/  {},
};
template <class T> consteval const syg_type_t* generate_component() {
  if constexpr (std::is_arithmetic_v<T>) return generate_value<T>();
  else                                   return &component_type_v<T>;
}


}
