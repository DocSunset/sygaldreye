// describe: a free function -> a node (descriptor + word), ticked through a binding.
#include "node.hpp"
#include "escapement.hpp"
#include <cassert>
using namespace syg::esc;

cell add(cell a, cell b) { return a + b; }

int main() {
  node d = describe<add>();
  assert(d.in_count == 2 && d.out_count == 1 && d.out_sizes[0] == sizeof(cell));

  cell a = 3, b = 4, o = 0;
  void* slots[] = { &a, &b, &o };        // first in_count are inputs, the rest outputs
  d.fn(slots);                           // the shim reads inputs and writes the output through slots
  assert(o == 7);
  return 0;
}
