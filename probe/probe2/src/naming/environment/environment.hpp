#pragma once
#include <map>
#include <set>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "address/address.hpp"

namespace syg::naming {

// The resolver's HERE (ADR-029): object store + ref table + subscriptions.
class environment {
 public:
  // Canonical-encode, hash, and hold a projection; returns the cid text.
  std::string commit(const nlohmann::json& projection);

  // Counted read: the first fetch of a cid is I/O, every later one is the
  // memo (NAM-2.1). Throws on an unknown cid.
  const nlohmann::json& fetch(const std::string& cid_text);
  int io_count() const { return io_count_; }

  void set_ref(const std::string& name, const std::string& cid_text);
  const std::string* ref(const std::string& name) const;

  void subscribe(const std::string& id, const std::string& address_text);
  // Rebind a ref; returns one event per live-address subscriber crossing it.
  std::vector<nlohmann::json> move_ref(const std::string& name,
                                       const std::string& cid_text);

 private:
  std::map<std::string, nlohmann::json> backing_;
  std::set<std::string> hot_;
  std::map<std::string, std::string> refs_;
  std::vector<std::pair<std::string, std::string>> subs_;  // id, address text
  int io_count_ = 0;
};

}  // namespace syg::naming
