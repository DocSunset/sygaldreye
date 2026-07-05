#include "naming_session.hpp"

#include <functional>
#include <stdexcept>

#include "environment/environment.hpp"
#include "resolver/resolver.hpp"
#include "spans/spans.hpp"

namespace syg::harness {
namespace {

bool is_placeholder(const nlohmann::json& v) {
  return v.is_object() && v.size() == 1 && v.contains("/") &&
         v["/"].is_string() && v["/"].get_ref<const std::string&>().starts_with("$");
}

}  // namespace

nlohmann::json naming_session(const nlohmann::json& input) {
  naming::environment env;
  std::map<std::string, std::string> cids;

  // commit objects in dependency order (links to "$name" force name first)
  std::function<std::string(const std::string&)> commit = [&](const std::string& name) {
    if (auto it = cids.find(name); it != cids.end()) return it->second;
    std::function<nlohmann::json(const nlohmann::json&)> subst =
        [&](const nlohmann::json& v) -> nlohmann::json {
      if (is_placeholder(v))
        return {{"/", commit(v["/"].get_ref<const std::string&>().substr(1))}};
      if (v.is_object() || v.is_array()) {
        auto out = v;
        for (auto& el : out) el = subst(el);
        return out;
      }
      return v;
    };
    return cids[name] = env.commit(subst(input.at("objects").at(name)));
  };
  for (const auto& [name, _] : input.at("objects").items()) commit(name);

  auto named_cid = [&](const std::string& s) {
    return s.starts_with("$") ? cids.at(s.substr(1)) : s;
  };
  const auto refs = input.value("refs", nlohmann::json::object());
  for (const auto& [name, target] : refs.items())
    env.set_ref(name, named_cid(target.get<std::string>()));

  nlohmann::json results = nlohmann::json::array();
  nlohmann::json events = nlohmann::json::array();
  for (const auto& op : input.value("ops", nlohmann::json::array())) {
    const std::string what = op.at("op");
    if (what == "resolve" || what == "normalize") {
      std::string text = op.at("addr");
      if (text.starts_with("$")) {  // "$name/route" → cid-rooted spelling
        auto slash = text.find('/');
        text = named_cid(text.substr(0, slash)) +
               (slash == std::string::npos ? "" : text.substr(slash));
      }
      int io_before = env.io_count();
      auto res = naming::resolve(env, syg::formats::parse_address(text));
      if (what == "normalize")
        results.push_back({{"normalized", res.normalized}});
      else
        results.push_back({{"fixity", res.live ? "live" : "fixed"},
                           {"normalized", res.normalized},
                           {"value", res.value},
                           {"io", env.io_count() - io_before}});
    } else if (what == "subscribe") {
      env.subscribe(op.at("as"), op.at("addr"));
      results.push_back(nullptr);
    } else if (what == "move-ref") {
      for (auto& e : env.move_ref(op.at("ref"), named_cid(op.at("to"))))
        events.push_back(e);
      results.push_back(nullptr);
    } else if (what == "span") {
      auto s = naming::span_at(env, env.fetch(named_cid(op.at("of"))),
                               op.at("at").at(0), op.at("at").at(1));
      results.push_back({{"span", {{"piece", s.piece_cid},
                                   {"start", s.start},
                                   {"len", s.len}}}});
    } else if (what == "span-text") {
      naming::span s{named_cid(op.at("span").at("piece")),
                     op.at("span").at("start"), op.at("span").at("len")};
      auto loc = naming::span_text(env, env.fetch(named_cid(op.at("of"))), s);
      results.push_back({{"text", loc.text}, {"position", loc.position}});
    } else if (what == "events") {
      results.push_back(events);
      events = nlohmann::json::array();
    } else {
      throw std::runtime_error("unknown naming op: " + what);
    }
  }
  return {{"cids", cids}, {"results", results}};
}

}  // namespace syg::harness
