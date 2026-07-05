#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>

#include <nlohmann/json.hpp>

#include "address/address.hpp"
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

int cmd_parse_address() {
  auto a = syg::formats::parse_address(read_stdin());
  static const char* names[] = {"refname", "cid", "peerkey"};
  nlohmann::ordered_json out{{"kind", names[static_cast<int>(a.kind)]},
                             {"head", a.head},
                             {"route", a.route}};
  std::cout << out.dump() << "\n";
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  std::string cmd = argc > 1 ? argv[1] : "";
  try {
    if (cmd == "encode") return cmd_encode();
    if (cmd == "parse-address") return cmd_parse_address();
  } catch (const std::exception& e) {
    std::cerr << "syg " << cmd << ": " << e.what() << "\n";
    return 1;
  }
  std::cerr << "syg: unknown subcommand '" << cmd << "' (see conformance/HARNESS.md)\n";
  return 2;
}
