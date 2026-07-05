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
#include "codecs.hpp"
#include "abi_audits.hpp"
#include "crown.hpp"
#include "hello_natives/hello_natives.hpp"
#include "naming_session.hpp"
#include "parser/parser.hpp"
#include "resolver/naive_resolver.hpp"
#include "hello_cosine/hello_cosine.hpp"
#include "oracle/oracle.hpp"

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

int cmd_connection_legal() {
  auto in = nlohmann::json::parse(read_stdin());
  auto p = [&](const char* side) {
    return syg::naming::promise{in.at(side).at(0), in.at(side).at(1)};
  };
  auto v = syg::naming::connection_legal(p("from"), p("to"));
  nlohmann::json out{{"legal", v.legal},
                     {"mapping", v.mapping.empty()
                                     ? nlohmann::json(nullptr)
                                     : nlohmann::json(v.mapping)}};
  std::cout << out.dump() << "\n";
  return 0;
}

void stdout_sink(void*, const float* block, int n) {
  std::fwrite(block, sizeof(float), static_cast<std::size_t>(n), stdout);
}

int cmd_render_movement(const std::string& fixture, double seconds) {
  if (fixture != "hello-cosine")
    throw std::runtime_error("unknown movement fixture: " + fixture);
  int frames = static_cast<int>(seconds * syg::movements::hello_cosine_rate);
  syg::movements::render_hello_cosine(frames, stdout_sink, nullptr);
  return 0;
}

int cmd_tick_audit() {
  // NAM-5.4: tick the frozen movement with the naming instrumentation live.
  long before = syg::naming::lookup_count();
  int frames = syg::movements::hello_cosine_rate;  // one second of ticks
  syg::movements::render_hello_cosine(
      frames, [](void*, const float*, int) {}, nullptr);
  nlohmann::json out{{"ticks", frames},
                     {"lookups", syg::naming::lookup_count() - before}};
  std::cout << out.dump() << "\n";
  return 0;
}

int cmd_codec_selftest(const std::string& type) {
  auto j = nlohmann::json::parse(read_stdin());
  nlohmann::json out;
  if (type == "widget_a") {
    syg::nodes::widget_a x;
    syg::generated::from_projection(j, x);
    out = syg::generated::to_projection(x);
  } else if (type == "widget_b") {
    syg::nodes::widget_b x;
    syg::generated::from_projection(j, x);
    out = syg::generated::to_projection(x);
  } else {
    throw std::runtime_error("no generated codec for " + type);
  }
  std::cout << out.dump() << "\n";
  return 0;
}

syg::crown::plan replayed_plan(const std::string& tape_text, int block) {
  syg::crown::plan p(syg::nodes::hello_natives(), block);
  for (auto& o : syg::crown::read_tape(tape_text)) p.submit(std::move(o));
  p.tick(0);  // the boot boundary: apply everything, process nothing yet
  return p;
}

int cmd_replay_tape() {
  auto p = replayed_plan(read_stdin(), 128);
  nlohmann::ordered_json nodes, defaults;
  nlohmann::json edges = nlohmann::json::array();
  for (const auto& r : p.nodes()) nodes[r.id] = {{"type", r.type}};
  for (const auto& [f, t] : p.edges()) edges.push_back({{"from", f}, {"to", t}});
  for (const auto& [route, v] : p.defaults()) {
    char* end = nullptr;
    double num = std::strtod(v.c_str(), &end);
    bool numeric = end && *end == 0 && end != v.c_str() &&
                   (v.find('.') != std::string::npos ||
                    v.find_first_not_of("-0123456789") == std::string::npos);
    if (numeric)
      defaults[route] = num;
    else
      defaults[route] = v;
  }
  std::cout << nlohmann::ordered_json{{"nodes", nodes}, {"edges", edges},
                                      {"defaults", defaults}}.dump() << "\n";
  return 0;
}

int cmd_render_tape(double seconds) {
  constexpr int block = syg::movements::hello_cosine_block;
  auto p = replayed_plan(read_stdin(), block);
  int frames = static_cast<int>(seconds * syg::movements::hello_cosine_rate);
  for (int done = 0; done < frames; done += block) {
    int n = frames - done < block ? frames - done : block;
    p.tick(n);
    std::fwrite(p.input_buffer("dac0", "in"), sizeof(float),
                static_cast<std::size_t>(n), stdout);
  }
  return 0;
}

int cmd_roundtrip() {
  auto g = syg::organs::parse_graph(nlohmann::json::parse(read_stdin()));
  std::cout << syg::organs::serialize_graph(g).dump(1) << "\n";
  return 0;
}

int cmd_resolve_hash(const std::string& cid, const std::string& objdir) {
  auto bytes = syg::organs::naive_resolve(objdir, cid);
  std::fwrite(bytes.data(), 1, bytes.size(), stdout);
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
    if (cmd == "connection-legal") return cmd_connection_legal();
    if (cmd == "render-movement" && argc > 3)
      return cmd_render_movement(argv[2], std::stod(argv[3]));
    if (cmd == "tick-audit") return cmd_tick_audit();
    if (cmd == "codec-selftest" && argc > 2) return cmd_codec_selftest(argv[2]);
    if (cmd == "hook-audit") return syg::harness::hook_audit();
    if (cmd == "create-audit" && argc > 2) return syg::harness::create_audit(argv[2]);
    if (cmd == "fault-audit") return syg::harness::fault_audit();
    if (cmd == "quarantine-audit") return syg::harness::quarantine_audit();
    if (cmd == "replay-tape") return cmd_replay_tape();
    if (cmd == "render-tape" && argc > 2) return cmd_render_tape(std::stod(argv[2]));
    if (cmd == "roundtrip") return cmd_roundtrip();
    if (cmd == "resolve-hash" && argc > 3) return cmd_resolve_hash(argv[2], argv[3]);
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
