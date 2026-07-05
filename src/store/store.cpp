#include "store.hpp"

#include <functional>
#include <stdexcept>

#include "dagcbor/dagcbor.hpp"
#include "pins/pins.hpp"

namespace syg::store {
namespace {

std::string cid_text_of(std::uint64_t codec, const formats::byte_vec& bytes) {
  return formats::cid_to_text(formats::cid_of(codec, bytes));
}

}  // namespace

std::string peer_store::put_raw(const formats::byte_vec& bytes, bool provide) {
  auto cid = cid_text_of(formats::pins::multicodec_raw, bytes);
  objects_[cid] = bytes;
  if (provide || provide_all_) provided_.insert(cid);
  return cid;
}

std::string peer_store::put_node(const nlohmann::json& projection, bool provide) {
  auto bytes = formats::encode_projection(projection);
  auto cid = cid_text_of(formats::pins::multicodec_dag_cbor, bytes);
  objects_[cid] = std::move(bytes);
  if (provide || provide_all_) provided_.insert(cid);
  return cid;
}

std::optional<formats::byte_vec> peer_store::get(const std::string& cid) const {
  auto it = objects_.find(cid);
  if (it == objects_.end()) return std::nullopt;  // a clean miss
  return it->second;
}

nlohmann::json peer_store::get_node(const std::string& cid) const {
  auto bytes = get(cid);
  if (!bytes) throw std::runtime_error("miss: " + cid);
  return formats::decode_to_projection(*bytes);
}

void peer_store::index_links(const std::string& provenance_cid,
                             const nlohmann::json& record) {
  // provenance points upstream; the store indexes the downstream inverse
  // as records are committed (STO-9)
  std::function<void(const nlohmann::json&)> walk = [&](const nlohmann::json& v) {
    if (v.is_object()) {
      if (v.size() == 1 && v.contains("/") && v["/"].is_string()) {
        backlinks_[v["/"].get<std::string>()].push_back(provenance_cid);
        return;
      }
      for (const auto& [k, x] : v.items()) walk(x);
    } else if (v.is_array()) {
      for (const auto& x : v) walk(x);
    }
  };
  walk(record);
}

std::optional<peer_store::committed> peer_store::memo_lookup(
    const nlohmann::json& recipe) const {
  auto bytes = formats::encode_projection(recipe);
  auto recipe_cid = cid_text_of(formats::pins::multicodec_dag_cbor, bytes);
  if (auto it = memo_.find(recipe_cid); it != memo_.end())
    return committed{it->second["output"], it->second["provenance"], true};
  return std::nullopt;
}

peer_store::committed peer_store::commit_derivation(
    nlohmann::json recipe, const nlohmann::json& output_projection) {
  auto recipe_cid = put_node(recipe, false);
  if (auto it = memo_.find(recipe_cid); it != memo_.end())
    return {it->second["output"], it->second["provenance"], true};
  auto output = put_node(output_projection, false);
  recipe["output"] = {{"/", output}};
  auto provenance = put_node(recipe, false);
  index_links(provenance, recipe);
  memo_[recipe_cid] = {{"output", output}, {"provenance", provenance}};
  return {output, provenance, false};
}

peer_store::committed peer_store::commit_capture(const formats::byte_vec& take,
                                                 nlohmann::json testimony) {
  // captures are provisional until provided; testimony is irreplaceable
  auto output = put_raw(take, false);
  testimony["output"] = {{"/", output}};
  auto provenance = put_node(testimony, false);
  index_links(provenance, testimony);
  return {output, provenance, false};
}

void peer_store::bind_ref(const std::string& name, const std::string& cid) {
  refs_[name].push_back(cid);
}

const std::string* peer_store::ref(const std::string& name) const {
  auto it = refs_.find(name);
  return it == refs_.end() || it->second.empty() ? nullptr : &it->second.back();
}

bool peer_store::undo_ref(const std::string& name) {
  auto it = refs_.find(name);
  if (it == refs_.end() || it->second.size() < 2) return false;
  // undo = rebind to the trail predecessor: an APPEND, never a pop — the
  // trail is history (ADR-018 at ref granularity)
  it->second.push_back(it->second[it->second.size() - 2]);
  return true;
}

const std::vector<std::string>& peer_store::trail(const std::string& name) const {
  static const std::vector<std::string> none;
  auto it = refs_.find(name);
  return it == refs_.end() ? none : it->second;
}

bool peer_store::provides(const std::string& cid) const {
  return provided_.count(cid) > 0;
}

void peer_store::evict_unprovided() {
  std::erase_if(objects_,
                [&](const auto& kv) { return !provided_.count(kv.first); });
}

void peer_store::unprovide(const std::string& cid) { provided_.erase(cid); }

const std::vector<std::string>& peer_store::backlinks(
    const std::string& cid) const {
  static const std::vector<std::string> none;
  auto it = backlinks_.find(cid);
  return it == backlinks_.end() ? none : it->second;
}

long fetch(peer_store& dst, peer_store& src, const std::string& root_cid,
           long stop_after) {
  long moved = 0;
  auto move_one = [&](const std::string& cid) {
    if (dst.get(cid)) return true;  // already here: nothing moves
    if (stop_after >= 0 && moved >= stop_after) return false;
    auto bytes = src.get(cid);
    if (!bytes) throw std::runtime_error("provider miss: " + cid);
    // verification is re-hashing under the cid's own multicodec
    auto raw_cid = cid_text_of(formats::pins::multicodec_raw, *bytes);
    auto cbor_cid = cid_text_of(formats::pins::multicodec_dag_cbor, *bytes);
    std::string got = cid == raw_cid   ? dst.put_raw(*bytes, false)
                      : cid == cbor_cid
                          ? dst.put_node(formats::decode_to_projection(*bytes),
                                         false)
                          : std::string();
    if (got != cid) throw std::runtime_error("verification failed for " + cid);
    ++moved;
    ++dst.transfer_counter();
    return true;
  };
  if (!move_one(root_cid)) return moved;
  // a chunk index fans out: fetch the missing chunks, resumable
  try {
    auto node = dst.get_node(root_cid);
    if (node.is_array()) {
      for (const auto& link : node) {
        if (!link.is_object() || !link.contains("/") || !link["/"].is_string())
          return moved;  // not an index node: done
      }
      for (const auto& link : node)
        if (!move_one(link["/"].get<std::string>())) return moved;
    }
  } catch (const std::exception&) {
    // a raw object: nothing to fan out
  }
  return moved;
}

}  // namespace syg::store
