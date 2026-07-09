#pragma once
#include <cstdlib>
#include "cell.hpp"

namespace syg::esc {

cell add(cell a, cell b) { return a + b; }
cell mul(cell a, cell b) { return a * b; }

cell less_than(cell a, cell b) { return a < b; }
cell select(cell c, cell a, cell b) { return c ? a : b; }

// fetch: read the cell at an address.
cell load(cell addr) { return *reinterpret_cast<cell*>(addr); }

cell mem_malloc(cell size) { return reinterpret_cast<cell>(malloc(size)); }

}
