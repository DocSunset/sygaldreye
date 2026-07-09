#pragma once
#include <cstddef>
#include <utility>
#include <type_traits>
#include "cell.hpp"

namespace syg::esc {

// A word is the executable form of a node: the shim the escapement calls. State is
// opaque bytes; the word is the ONLY place that knows the types. It gets ONE array
// of cell pointers — the first in_count are inputs, the rest outputs (inputs then
// outputs, like a component's members) — and reads/writes through them.
using word = void (*)(void** slots);

// A node is a declaration: a word plus what the planner needs to lay out state —
// how many inputs and outputs, and how big each is. This struct is ONE reification
// of a node in its declaration role; the binding (escapement.hpp) reifies the
// binding role, and the instance is a node left un-reified entirely.
struct node {
  std::size_t        in_count;
  const std::size_t* in_sizes;
  std::size_t        out_count;
  const std::size_t* out_sizes;
  word               fn;
};

template <auto Fn, class R, class... Args>
node describe_(R (*)(Args...)) {
  // A word must not SILENTLY mutate its inputs (fan-out cells are shared, L8).
  // The one surprising mutation surface is a mutable reference — reads like
  // by-value yet writes back — so the only reference form allowed is a const
  // LVALUE reference (const T&). An rvalue ref (T&&) means move-from, but an
  // input cell is shared and persists — you can't pilfer it — so it's out too.
  // By value is a copy (safe); a pointer is a deliberate "going raw" (Rust's
  // unsafe): the cell holds an address, the word gets it by value, what it does
  // through it is its business.
  static_assert(
    ( ... && (!std::is_reference_v<Args>
              || (std::is_lvalue_reference_v<Args>
                  && std::is_const_v<std::remove_reference_t<Args>>)) ),
    "a word's reference input must be a const lvalue reference (const T&) — "
    "otherwise take it by value, or by pointer if you mean to go raw");

  static constexpr std::size_t ins[]  = { sizeof(Args)..., 0 };  // trailing 0: never a 0-size array
  // a free function has exactly one output — its return — or none, if void
  static constexpr std::size_t outs[] = { sizeof(std::conditional_t<std::is_void_v<R>, char, R>), 0 };
  return {
    sizeof...(Args), ins, std::size_t(std::is_void_v<R> ? 0 : 1), outs,
    [](void** slots) {
      [=]<std::size_t... I>(std::index_sequence<I...>) {
        if constexpr (std::is_void_v<R>)
          Fn(*static_cast<std::remove_reference_t<Args>*>(slots[I])...);
        else                                                         // output sits at slots[in_count]
          *static_cast<R*>(slots[sizeof...(Args)]) = Fn(*static_cast<std::remove_reference_t<Args>*>(slots[I])...);
      }(std::index_sequence_for<Args...>{});
    }
  };
}

template <auto Fn> node describe() { return describe_<Fn>(Fn); }

}
