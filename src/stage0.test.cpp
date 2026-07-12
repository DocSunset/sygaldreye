// describe_function<^^fn>: a free function -> a stage-0 node, with names and
// namespaces harvested by reflection, and outputs splayed when the return type is
// annotated [[=outputs{}]]. assert-based; exit 0 is a pass. Needs -freflection.
#include "stage0.hpp"
#include <cassert>
#include <cstdint>
using namespace syg::stage0;

namespace demo::math {
  double add(double a, double b) { return a + b; }
  void   sink(double a) { (void)a; }
}

// component<^^fn> / describe_component<C>: the type-rich twin and its erasure.
using cell = std::intptr_t;
namespace comp {
  cell   add(cell a, cell b)                 { return a + b; }
  double blend(double x, double y, double t) { return x * (1 - t) + y * t; }
  struct AddC   { const cell& a; const cell& b; cell out; void operator()() { out = a + b; } };
  struct SplitC { const cell& x; cell lo; cell hi; void operator()() { lo = x / 2; hi = x - x / 2; } };
}

// a return type annotated as multiple outputs — same shape as a plain struct, the
// annotation is what turns its members into two named output cells.
struct [[=outputs{}]] divmod_out { int q; int r; };
namespace demo::math { divmod_out idivmod(int a, int b) { return { a / b, a % b }; } }

int main() {
  // single output: name, inputs, the lone "out", and the namespace chain.
  node n = describe_function<^^demo::math::add>();
  assert(n.name == "add");
  assert(n.in_sizes.size() == 2 && n.in_names[0] == "a" && n.in_names[1] == "b");
  assert(n.out_sizes.size() == 1 && n.out_names[0] == "out");
  assert(n.namespaces.size() == 2 && n.namespaces[0] == "demo" && n.namespaces[1] == "math");
  double a = 3, b = 4, out = 0;  void* s[] = { &a, &b, &out };
  n.fn(s);
  assert(out == 7);

  // void return: no output cells at all.
  node v = describe_function<^^demo::math::sink>();
  assert(v.out_sizes.size() == 0 && v.out_names.size() == 0);

  // splayed return: two outputs named after the struct members, written per-member.
  node d = describe_function<^^demo::math::idivmod>();
  assert(d.out_sizes.size() == 2 && d.out_names[0] == "q" && d.out_names[1] == "r");
  int ia = 17, ib = 5, q = 0, r = 0;  void* ds[] = { &ia, &ib, &q, &r };
  d.fn(ds);
  assert(q == 3 && r == 2);

  // component<> generates a typed struct with named members + a call operator.
  cell ca = 3, cb = 4;
  component<^^comp::add> A{{ ca, cb, 0 }};        // inputs a, b; owned output out = 0
  A();
  assert(A.a == 3 && A.out == 7);

  double dx = 10, dy = 20, dt = 0.25;
  component<^^comp::blend> B{{ dx, dy, dt, 0.0 }};
  B();
  assert(B.out == 12.5);

  // describe_component<> erases an authored component back to a node.
  node dc = describe_component<comp::AddC>();
  cell ea = 3, eb = 4, eo = 0;  void* sc[] = { &ea, &eb, &eo };
  dc.fn(sc);
  assert(dc.in_sizes.size() == 2 && dc.out_sizes.size() == 1 && eo == 7);
  assert(dc.name == "AddC" && dc.namespaces.size() == 1 && dc.namespaces[0] == "comp");
  assert(dc.in_names[0] == "a" && dc.in_names[1] == "b" && dc.out_names[0] == "out");

  node ds2 = describe_component<comp::SplitC>();  // multiple outputs from a component
  cell xin = 10, lo = 0, hi = 0;  void* ss[] = { &xin, &lo, &hi };
  ds2.fn(ss);
  assert(ds2.in_sizes.size() == 1 && ds2.out_sizes.size() == 2 && lo == 5 && hi == 5);
  assert(ds2.name == "SplitC" && ds2.in_names[0] == "x");
  assert(ds2.out_names[0] == "lo" && ds2.out_names[1] == "hi");
  return 0;
}
