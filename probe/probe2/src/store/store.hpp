#pragma once
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "cid/cid.hpp"

namespace syg::store {

class peer_store {
 public:
  explicit peer_store(std::string name, bool provide_all = false)
      : name_(std::move(name)), provide_all_(provide_all) {}

  // object machinery (STO-1): content-addressed; get of unknown = nullopt
  std::string put_raw(const formats::byte_vec& bytes, bool provide);
  std::string put_node(const nlohmann::json& projection, bool provide);
  std::optional<formats::byte_vec> get(const std::string& cid) const;
  nlohmann::json get_node(const std::string& cid) const;  // decode; throws on miss

  // the two commit paths (STO-3): both index back-links at commit (STO-9)
  // derive: recipe {op, inputs{name:{"/":cid}}, ...}; memoized by recipe cid
  struct committed {
    std::string output, provenance;
    bool memo_hit = false;
  };
  committed commit_derivation(nlohmann::json recipe,
                              const nlohmann::json& output_projection);
  // memo probe WITHOUT committing anything (the caller decides whether to
  // run passes at all)
  std::optional<committed> memo_lookup(const nlohmann::json& recipe) const;
  // capture: testimony {peer-key, wiring-route, free-form}; irreplaceable
  committed commit_capture(const formats::byte_vec& take,
                           nlohmann::json testimony);

  // refs (STO-4): rebind appends; undo rebinds to the trail predecessor
  void bind_ref(const std::string& name, const std::string& cid);
  const std::string* ref(const std::string& name) const;
  bool undo_ref(const std::string& name);
  const std::vector<std::string>& trail(const std::string& name) const;

  // availability (STO-5/8)
  bool provides(const std::string& cid) const;
  void evict_unprovided();                  // cache pressure
  void unprovide(const std::string& cid);   // forgetting is a decision

  const std::vector<std::string>& backlinks(const std::string& cid) const;
  const std::map<std::string, formats::byte_vec>& objects() const {
    return objects_;
  }
  const std::string& name() const { return name_; }
  long& transfer_counter() { return transfers_; }

 private:
  void index_links(const std::string& provenance_cid,
                   const nlohmann::json& record);
  std::string name_;
  bool provide_all_;
  std::map<std::string, formats::byte_vec> objects_;
  std::set<std::string> provided_;
  std::map<std::string, std::vector<std::string>> refs_;
  std::map<std::string, std::vector<std::string>> backlinks_;
  std::map<std::string, nlohmann::json> memo_;  // recipe cid -> result
  long transfers_ = 0;
};

// Chunk-aware transfer: moves the root and any missing chunks from a
// provider to dst; stops after `stop_after` objects when nonnegative
// (the interruption); returns objects moved. Retrying moves only what is
// still missing (STO-6.1).
long fetch(peer_store& dst, peer_store& src, const std::string& root_cid,
           long stop_after = -1);

}  // namespace syg::store
