#pragma once

namespace syg::esc {

// make_binding: a self-owning binding — one heap blob, [fn][slots][outmem]. Inputs
// start unwired (nullptr); each output points at an owned cell just past the slot
// array. The binding owns its output the way a component owns `T out`; the only
// difference is placement, forced by erasure — the size isn't known until runtime.
// (Outputs pack flat after the 8-aligned slot array: fine for cell-shaped cells.)
binding* make_binding(const node* n) {
  std::size_t nin = n->in_sizes.size();
  std::size_t nout = n->out_sizes.size();
  std::size_t outbytes = 0;
  for (std::size_t sz : n->out_sizes) outbytes += sz;
  binding* b = (binding*)std::malloc(sizeof(binding) + (nin + nout) * sizeof(void*) + outbytes);
  b->fn = n->fn;
  void** s = b->slots();
  unsigned char* out = (unsigned char*)(s + nin + nout);
  for (std::size_t j = 0; j < nin;  ++j) s[j] = nullptr;                          // inputs unwired
  for (std::size_t j = 0; j < nout; ++j) { s[nin + j] = out; out += n->out_sizes[j]; }
  return b;
}

// A binding is one blob: [ fn ][ slots ][ outmem ]. The slots array (first
// in_count are inputs, the rest outputs) lives INLINE right after fn, and the
// output slots point into outmem — the owned output cells, later in the same blob.
struct binding {
  word fn;
  // [slots], if any
  // [outputs memory], if any
  void operator()() const { fn((void**)(this+1)); }
};

}
