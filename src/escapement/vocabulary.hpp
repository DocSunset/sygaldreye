#pragma once
#include <cstdlib>
#include "cell.hpp"

namespace syg::esc {

cell add(cell a, cell b) { return a + b; }
cell mul(cell a, cell b) { return a * b; }

cell less_than(cell a, cell b) { return a < b; }
cell equal(cell a, cell b) { return a == b; }
cell select(cell c, cell a, cell b) { return c ? a : b; }

// fetch: read the cell at an address.
cell load(cell addr) { return *reinterpret_cast<cell*>(addr); }

cell mem_malloc(cell size) { return reinterpret_cast<cell>(malloc(size)); }

cell loop_count(cell count, cell step) {
    cell next = add(count, 1);
    cell loop = equal(steps, next);
    return select(loop, 0, next);
}

struct loop_counter {
    cell steps;
    cell count;

    void init(cell s) {
        cell negative = less_than(s, 0);
        steps = select(negative, -s, s);
        count = 0;
    }

    void operator()() {
        count = loop_counter(steps, count);
    }
};
