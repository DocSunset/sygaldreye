// crown: replay a tape into a movement of self-owning bindings, then tick it.
#include "crown.hpp"
#include <cassert>
#include <cstdlib>
#include <cstring>
using namespace syg::esc;

cell add(cell a, cell b) { return a + b; }

int main() {
  node dict[] = { describe<add>(), constant() };   // type 0 = add, type 1 = constant
  op tape[] = {                    // (3 + 4) + 10, constants are nodes
    {'N', 1, 3,  0, 0},            // inst 0 = const 3
    {'N', 1, 4,  0, 0},            // inst 1 = const 4
    {'N', 0, 0,  0, 0},            // inst 2 = add
    {'N', 1, 10, 0, 0},            // inst 3 = const 10
    {'N', 0, 0,  0, 0},            // inst 4 = add
    {'L', 0, 0, 2, 0},             // const3 -> add2 in0
    {'L', 1, 0, 2, 1},             // const4 -> add2 in1
    {'L', 2, 0, 4, 0},             // (3+4)  -> add4 in0
    {'L', 3, 0, 4, 1},             // const10-> add4 in1
  };
  unsigned char* arena = (unsigned char*)std::malloc(4096);
  plan pl = crown(arena, 4096, dict, 2, tape, 9);
  tick(pl.steps, pl.movement);

  cell r; std::memcpy(&r, pl.movement[4]->slots[2], sizeof r);   // add's output at slots[in_count]
  assert(r == 17);
  std::free(arena);
  return 0;
}
