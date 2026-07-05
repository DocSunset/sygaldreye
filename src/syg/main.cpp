#include <cstdio>
#include <map>
#include <iostream>
#include <sstream>
#include <string>

#include <nlohmann/json.hpp>

#include "address/address.hpp"
#include "chunk/chunk.hpp"
#include "cid/cid.hpp"
#include "pins/pins.hpp"
#include "dagcbor/dagcbor.hpp"
#include "naming_session.hpp"

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

int cmd_hash() {
  auto in = read_stdin();
  syg::formats::byte_vec data(in.begin(), in.end());
  std::cout << syg::formats::cid_to_text(
                   syg::formats::cid_of(syg::formats::pins::multicodec_raw, data))
            << "\n";
  return 0;
}

int cmd_verify(const std::string& cid_text) {
  auto in = read_stdin();
  syg::formats::byte_vec data(in.begin(), in.end());
  bool ok = false;
  try {
    ok = syg::formats::cid_verify(syg::formats::cid_from_text(cid_text), data);
  } catch (const std::exception&) {
  }
  std::cout << (ok ? "{\"ok\":true}" : "{\"ok\":false}") << "\n";
  return 0;
}

int cmd_chunk_put() {
  auto in = nlohmann::json::parse(read_stdin());
  std::map<syg::formats::byte_vec, std::size_t> objects;  // cid -> size
  nlohmann::json roots = nlohmann::json::array();
  for (const auto& blob : in.at("blobs")) {
    auto root = syg::formats::chunk_put(
        syg::formats::bytes_of_projection(blob),
        [&](const syg::formats::byte_vec& cid, const syg::formats::byte_vec& obj) {
          objects.emplace(cid, obj.size());
        });
    roots.push_back(syg::formats::cid_to_text(root));
  }
  std::size_t stored = 0;
  for (const auto& [cid, size] : objects) stored += size;
  nlohmann::ordered_json out{
      {"roots", roots}, {"objects", objects.size()}, {"stored_bytes", stored}};
  std::cout << out.dump() << "\n";
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
    if (cmd == "hash") return cmd_hash();
    if (cmd == "verify" && argc > 2) return cmd_verify(argv[2]);
    if (cmd == "chunk-put") return cmd_chunk_put();
    if (cmd == "naming") {
      std::cout << syg::harness::naming_session(nlohmann::json::parse(read_stdin())).dump() << "\n";
      return 0;
    }
  } catch (const std::exception& e) {
    std::cerr << "syg " << cmd << ": " << e.what() << "\n";
    return 1;
  }
  std::cerr << "syg: unknown subcommand '" << cmd << "' (see conformance/HARNESS.md)\n";
  return 2;
}
