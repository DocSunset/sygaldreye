// component<^^fn>: a function -> a type-rich node (needs -freflection).
// describe_component<C>: an authored component -> a type-erased node.
#include "component.hpp"
#include <cassert>
using namespace syg::esc;

cell   add(cell a, cell b)                { return a + b; }
double blend(double x, double y, double t) { return x * (1 - t) + y * t; }

struct AddC   { const cell& a; const cell& b; cell out; void operator()() { out = a + b; } };
struct SplitC { const cell& x; cell lo; cell hi; void operator()() { lo = x / 2; hi = x - x / 2; } };

int main() {
  // component<> generates a typed struct with named members + a call operator.
  cell a = 3, b = 4;
  component<^^add> A{{a, b, 0}};                 // inputs a, b; owned output out = 0
  A();
  assert(A.a == 3 && A.out == 7);

  double x = 10, y = 20, t = 0.25;
  component<^^blend> B{{x, y, t, 0.0}};
  B();
  assert(B.out == 12.5);

  // describe_component<> erases an authored component back to a node.
  node d = describe_component<AddC>();
  cell ia = 3, ib = 4, io = 0;  void* s[] = { &ia, &ib, &io };
  d.fn(s);
  assert(d.in_sizes.size() == 2 && d.out_sizes.size() == 1 && io == 7);

  node d2 = describe_component<SplitC>();   // multiple outputs from a component
  cell xin = 10, lo = 0, hi = 0;  void* s2[] = { &xin, &lo, &hi };
  d2.fn(s2);
  assert(d2.in_sizes.size() == 1 && d2.out_sizes.size() == 2 && lo == 5 && hi == 5);
  return 0;
}
