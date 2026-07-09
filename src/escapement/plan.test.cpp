// make_binding: a self-owning binding in an arena — inputs unwired, outputs owned.
#include "plan.hpp"
#include <cassert>
#include <cstdlib>
#include <cstring>
using namespace syg::esc;

cell add(cell a, cell b) { return a + b; }

int main() {
  node dict[] = { describe<add>() };

  binding* b = make_binding(&dict[0]);                            // self-owning: it mallocs its own blob
  assert(b->slots()[0] == nullptr && b->slots()[1] == nullptr);   // inputs start unwired

  cell a = 3, c = 4;
  b->slots()[0] = &a; b->slots()[1] = &c;                         // wire the inputs
  (*b)();
  cell r; std::memcpy(&r, b->slots()[2], sizeof r);              // owned output at slots[in_count]
  assert(r == 7);

  node k = constant();                                         // a source node owns its value
  assert(k.in_sizes.size() == 0 && k.out_sizes.size() == 1 && k.out_sizes[0] == sizeof(cell));

  std::free(b);
  return 0;
}
