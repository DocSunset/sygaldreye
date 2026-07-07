#include "environment.hpp"

#include <stdexcept>

#include "cid/cid.hpp"
#include "dagcbor/dagcbor.hpp"
#include "pins/pins.hpp"

namespace syg::naming {

std::string environment::commit(const nlohmann::json& projection) {
  auto bytes = formats::encode_projection(projection);
  auto text = formats::cid_to_text(
      formats::cid_of(formats::pins::multicodec_dag_cbor, bytes));
  backing_.emplace(text, projection);
  return text;
}

const nlohmann::json& environment::fetch(const std::string& cid_text) {
  auto it = backing_.find(cid_text);
  if (it == backing_.end())
    throw std::runtime_error("no such object: " + cid_text);
  if (hot_.insert(cid_text).second) ++io_count_;
  return it->second;
}

void environment::set_ref(const std::string& name, const std::string& cid_text) {
  refs_[name] = cid_text;
}

const std::string* environment::ref(const std::string& name) const {
  auto it = refs_.find(name);
  return it == refs_.end() ? nullptr : &it->second;
}

void environment::subscribe(const std::string& id,
                            const std::string& address_text) {
  subs_.emplace_back(id, address_text);
}

std::vector<nlohmann::json> environment::move_ref(const std::string& name,
                                                  const std::string& cid_text) {
  refs_[name] = cid_text;
  std::vector<nlohmann::json> events;
  for (const auto& [id, text] : subs_) {
    auto addr = formats::parse_address(text);
    if (addr.kind == formats::address::first_step::refname && addr.head == name)
      events.push_back({{"to", id}, {"addr", text}, {"now", cid_text}});
  }
  return events;
}

}  // namespace syg::naming
