// The instrumented allocator (EXE-3.1 / ABI-2.1): every global allocation
// in the harness binary is counted against the current hook phase.
#include <cstdlib>
#include <new>

#include "phase.hpp"

void* operator new(std::size_t n) {
  syg::abi::count_alloc();
  if (void* p = std::malloc(n ? n : 1)) return p;
  throw std::bad_alloc();
}

void operator delete(void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
