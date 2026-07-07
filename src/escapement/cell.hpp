#include <cstdint>

namespace syg::esc {

// one cell = one machine word: wide enough to hold an integer or a pointer.
using cell = std::intptr_t;

}
