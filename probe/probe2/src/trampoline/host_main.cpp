// SZ-6 trampoline: the sealed hello-cosine firmware's entire platform entry.
#include <cstdio>
#include <cstdlib>
#include "hello_cosine/hello_cosine.hpp"
static void sink(void*, const float* b, int n) { std::fwrite(b, 4, static_cast<size_t>(n), stdout); }
int main(int argc, char** argv) {
  int seconds = argc > 1 ? std::atoi(argv[1]) : 8;
  syg::movements::render_hello_cosine(seconds * syg::movements::hello_cosine_rate, sink, nullptr);
}
