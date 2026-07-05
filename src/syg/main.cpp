#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>

#include <nlohmann/json.hpp>

#include "address/address.hpp"
#include "pins/pins.hpp"
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

int cmd_pins() {
  namespace p = syg::formats::pins;
  nlohmann::ordered_json out;
  out["multicodec"] = {{"dag-cbor", p::multicodec_dag_cbor},
                       {"raw", p::multicodec_raw}};
  out["multihash"] = {{"blake3-256", p::multihash_blake3_256}};
  out["hash_bytes"] = p::hash_bytes;
  out["multibase"] = std::string(1, p::multibase_prefix);
  out["chunk_bytes"] = p::chunk_bytes;
  out["escape"] = p::escape_set;
  out["tape_records"] = p::tape_records;
  out["edit_ops"] = p::edit_ops;
  nlohmann::ordered_json wire;
  for (std::size_t i = 0; i < p::wire_kinds.size(); ++i)
    wire[std::string(p::wire_kinds[i])] = i + 1;
  out["wire_kinds"] = wire;
  std::cout << out.dump() << "\n";
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
    if (cmd == "pins") return cmd_pins();
  } catch (const std::exception& e) {
    std::cerr << "syg " << cmd << ": " << e.what() << "\n";
    return 1;
  }
  std::cerr << "syg: unknown subcommand '" << cmd << "' (see conformance/HARNESS.md)\n";
  return 2;
}
