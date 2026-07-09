#pragma once
// Generates a type-rich binding from a free function, via C++26 reflection.
// `component<^^add>` is a struct shaped like an authored native node: a const T&
// per parameter (named after it) to reference upstream outputs, an OWNED T out it
// produces, and an operator() that runs the function. This is the thesis's
// "component" — the type-rich twin of the escapement's type-erased binding (which
// is the same shape with everything as void* pointers). REQUIRES -freflection.
//
// Three rules the compiler enforces, encoded below:
//   1. define_aggregate must run from a `consteval { }` BLOCK, never a function.
//   2. that block must share the target type's scope — hence `data` is NESTED.
//   3. splicing a std::meta::info in a runtime body escalates it to consteval —
//      so Fn becomes a function pointer once, and member infos ride as template
//      arguments (a constant context), never as runtime values.
#include <meta>
#include <utility>
#include <vector>
#include "node.hpp"

namespace syg::esc {

template <std::meta::info Fn>
struct component_generator {
  // Same input-purity rule as describe (node.hpp): a node must not silently mutate
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
    mem.push_back(m::data_member_spec(            // OWNS its output (T out), like an authored node
        m::return_type_of(Fn), {.name = "out"}));
    m::define_aggregate(^^data, mem);
  }

  static constexpr auto fn = &[: Fn :];           // Fn spliced once -> runtime fn pointer (rule 3)

  struct generated_component : data {             // inherit the members: access is A.a, not A.d.a
    template <std::meta::info Out, std::meta::info... Ins>
    void run() { this->[: Out :] = fn(this->[: Ins :]...); }   // out = fn(a, b) — mutates owned out

    void operator()() {
      static constexpr auto mem = std::define_static_array(
          std::meta::nonstatic_data_members_of(^^data, std::meta::access_context::current()));
      constexpr std::size_t n = mem.size() - 1;   // last member is out; the rest are inputs
      [this]<std::size_t... I>(std::index_sequence<I...>) {
        this->template run<mem[n], mem[I]...>();  // infos as template args (rule 3)
      }(std::make_index_sequence<n>{});
    }
  };
};

// describe_component: the reverse of component<> — erase an authored component
// type into a type-erased node. Reference members are inputs (referenced
// upstream), value members are owned outputs (inputs must precede outputs in
// declaration order). The shim constructs C over the input cells, ticks it, and
// writes each owned output to its cell — the copy elides (C never escapes).
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
  v.push_back(0);   // never a 0-size array
  return v;
}

template <typename C>
node describe_component() {
  namespace m = std::meta;
  static constexpr auto ins   = std::define_static_array(comp_members(^^C, true));
  static constexpr auto outs  = std::define_static_array(comp_members(^^C, false));
  static constexpr auto insz  = std::define_static_array(comp_sizes(^^C, true));
  static constexpr auto outsz = std::define_static_array(comp_sizes(^^C, false));
  return {
    ins.size(), insz.data(), outs.size(), outsz.data(),
    [](void** slots) {
      constexpr std::size_t nin = ins.size();            // outputs start at slots[in_count]
      C c = [&]<std::size_t... I, std::size_t... J>(std::index_sequence<I...>, std::index_sequence<J...>) {
        return C{ *static_cast<typename [: m::remove_reference(m::type_of(ins[I])) :]*>(slots[I])...,
                  (typename [: m::type_of(outs[J]) :]){}... };   // value-init owned outputs
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

}
