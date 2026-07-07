#include <cstddef>
#include "cell.hpp"
#include "node.hpp"
#include "vocabulary.hpp"

namespace syg::esc {

// A wiring is one call: which word (index into the dictionary), pointers to its
// input cells, and where to write its outputs.
struct wiring {
  cell   word;
  cell** inputs;
  cell*  outputs;
};

// The counter-driven reference form.
void tick(const node* dict, const wiring* movement, cell steps) {
  for (cell i = 0; i < steps; ++i)
    dict[movement[i].word].fn(movement[i].inputs, movement[i].outputs);
}

// The same thing, refactored so its body holds only assignments and word-calls:
// every operator (<, [], ., ++, the indirect call) is now a word. dict and
// movement arrive as cells (addresses); the ground `while` stays fiat C++.
void escapement(cell dict, cell movement, cell steps) {
  cell wsize = sizeof(wiring);
  cell nsize = sizeof(node);
  cell w_in  = offsetof(wiring, inputs);
  cell w_out = offsetof(wiring, outputs);
  cell n_fn  = offsetof(node, fn);

  cell pc = 0;
  cell cont = less_than(pc, steps);
  while (cont) {
    cell woff    = mul(pc, wsize);
    cell waddr   = add(movement, woff);
    cell word    = load(waddr);
    cell ioff    = add(waddr, w_in);
    cell inputs  = load(ioff);
    cell ooff    = add(waddr, w_out);
    cell outputs = load(ooff);
    cell noff    = mul(word, nsize);
    cell naddr   = add(dict, noff);
    cell foff    = add(naddr, n_fn);
    cell fn      = load(foff);
    call(fn, inputs, outputs);
    pc   = add(pc, 1);
    cont = less_than(pc, steps);
  }
}

}
