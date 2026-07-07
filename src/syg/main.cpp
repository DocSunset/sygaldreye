// The syg CLI: one entrypoint, one subcommand per rung. Each subcommand reads
// stdin, writes stdout, exits (conformance/HARNESS.md). Rung 1 lands the first
// real ones (encode, hash, parse-address, ...); until then this is the frame.
#include <cstring>
#include <iostream>

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << "usage: syg <subcommand> (see conformance/HARNESS.md)\n";
    return 2;
  }
  std::cerr << "syg: unknown subcommand '" << argv[1]
            << "' (see conformance/HARNESS.md)\n";
  return 2;
}
