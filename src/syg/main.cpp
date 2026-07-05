#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>

#include <nlohmann/json.hpp>

#include "dagcbor/dagcbor.hpp"

namespace {

std::string read_stdin() {
  std::ostringstream ss;
  ss << std::cin.rdbuf();
  return ss.str();
}

int cmd_encode() {
  auto v = nlohmann::json::parse(read_stdin());
  auto out = syg::formats::encode_projection(v);
  std::fwrite(out.data(), 1, out.size(), stdout);
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  std::string cmd = argc > 1 ? argv[1] : "";
  try {
    if (cmd == "encode") return cmd_encode();
  } catch (const std::exception& e) {
    std::cerr << "syg " << cmd << ": " << e.what() << "\n";
    return 1;
  }
  std::cerr << "syg: unknown subcommand '" << cmd << "' (see conformance/HARNESS.md)\n";
  return 2;
}
