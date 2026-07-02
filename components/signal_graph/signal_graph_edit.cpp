// Copyright 2026 Travis West
// Structured edit ops (vr_editor_decomposition.md S2): the editor's
// whole-graph JSON splicing, moved out of vr_editor_interactions.cpp into one
// tested place. Each op is applied as a transform on the serialized graph.
// Matching is structural (per array object, key at object top level) so a
// param VALUE containing "id":"x" can't hijack an op; op fields unescape on
// read and re-escape on write (gesture nodes escape via
// editor_layout::json_escape).
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "signal_graph.hpp"

namespace {

std::string unescape(std::string_view s) {
    std::string out;
    for (std::size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '\\' && i + 1 < s.size()) ++i;
        out += s[i];
    }
    return out;
}

std::string escape(std::string_view s) {
    std::string out;
    for (char c : s) {
        if (c == '"' || c == '\\') out += '\\';
        out += c;
    }
    return out;
}

// Escape-aware "key":"value" extraction (op JSON is flat), unescaped.
std::string str_field(std::string_view json, std::string_view key) {
    std::string needle = "\"";
    needle += key;
    needle += "\":\"";
    auto p = json.find(needle);
    if (p == std::string_view::npos) return {};
    p += needle.size();
    auto e = p;
    while (e < json.size() && json[e] != '"') e += (json[e] == '\\') ? 2 : 1;
    if (e > json.size()) return {};
    return unescape(json.substr(p, e - p));
}

// String field at the TOP level of one object slice (skips nested objects/
// arrays and string values), unescaped. Empty if absent or non-string.
std::string top_field(std::string_view obj, std::string_view key) {
    auto skip_str = [&](std::size_t i) {  // i at opening quote → closing quote
        for (++i; i < obj.size() && obj[i] != '"'; i += (obj[i] == '\\') ? 2 : 1) {}
        return i;
    };
    int depth = 0;
    for (std::size_t i = 0; i < obj.size(); ++i) {
        char c = obj[i];
        if (c == '{' || c == '[')
            ++depth;
        else if (c == '}' || c == ']')
            --depth;
        else if (c == '"') {
            if (depth != 1) {  // string inside a nested value
                i = skip_str(i);
                continue;
            }
            std::size_t ks = i + 1, ke = skip_str(i);
            i = ke;
            std::size_t j = ke + 1;
            if (j >= obj.size() || obj[j] != ':') continue;
            ++j;
            bool str_val = j < obj.size() && obj[j] == '"';
            if (obj.substr(ks, ke - ks) == key)
                return str_val ? unescape(obj.substr(j + 1, skip_str(j) - j - 1)) : std::string{};
            if (str_val) i = skip_str(j);  // don't misread the value as a key
        }
    }
    return {};
}

// [begin, end] (inclusive) of each object in the ROOT-level array keyed
// `key`. Key lookup is depth-aware: an inline subgraph's nested "nodes"/
// "edges" arrays must not shadow the document's.
std::vector<std::pair<std::size_t, std::size_t>> array_objects(
    const std::string& s, std::string_view key) {
    std::vector<std::pair<std::size_t, std::size_t>> out;
    auto skip_str = [&](std::size_t i) {  // i at opening quote → closing quote
        for (++i; i < s.size() && s[i] != '"'; i += (s[i] == '\\') ? 2 : 1) {}
        return i;
    };
    std::size_t p = std::string::npos;
    int depth = 0;
    for (std::size_t i = 0; i < s.size() && p == std::string::npos; ++i) {
        char c = s[i];
        if (c == '{' || c == '[')
            ++depth;
        else if (c == '}' || c == ']')
            --depth;
        else if (c == '"') {
            std::size_t ks = i + 1, ke = skip_str(i);
            i = ke;
            if (depth == 1 && ke + 1 < s.size() && s[ke + 1] == ':') {
                if (std::string_view(s).substr(ks, ke - ks) == key && ke + 2 < s.size() &&
                    s[ke + 2] == '[')
                    p = ke + 3;
                else if (ke + 2 < s.size() && s[ke + 2] == '"')
                    i = skip_str(ke + 2);  // don't misread the value as a key
            }
        }
    }
    if (p == std::string::npos) return out;
    int d = 0;
    std::size_t start = 0;
    for (std::size_t i = p; i < s.size(); ++i) {
        char c = s[i];
        if (c == '"')
            i = skip_str(i);
        else if (c == '{') {
            if (d++ == 0) start = i;
        } else if (c == '}') {
            if (--d == 0) out.emplace_back(start, i);
        } else if (c == ']' && d == 0)
            break;
    }
    return out;
}

// Erase spans (plus one adjacent comma each), back to front.
void erase_spans(std::string& s, std::vector<std::pair<std::size_t, std::size_t>> spans) {
    for (auto it = spans.rbegin(); it != spans.rend(); ++it) {
        auto [b, e] = *it;
        if (b > 0 && s[b - 1] == ',')
            --b;
        else if (e + 1 < s.size() && s[e + 1] == ',')
            ++e;
        s.erase(b, e - b + 1);
    }
}

std::string_view slice(const std::string& s, std::pair<std::size_t, std::size_t> span) {
    return std::string_view(s).substr(span.first, span.second - span.first + 1);
}

// Node part of an "n.p" endpoint (parse_graph splits on the first dot).
std::string_view node_of(std::string_view endpoint) {
    return endpoint.substr(0, endpoint.find('.'));
}

// Append `entry` at the end of the named array.
void insert_in_array(std::string& s, std::string_view array_key, const std::string& entry) {
    auto objs = array_objects(s, array_key);
    std::string needle = "\"";
    needle += array_key;
    needle += "\":[";
    auto kp = s.find(needle);
    if (kp == std::string::npos) return;
    if (objs.empty())
        s.insert(kp + needle.size(), entry);
    else
        s.insert(objs.back().second + 1, "," + entry);
}

void remove_node(std::string& s, const std::string& id) {
    if (id.empty()) return;
    std::vector<std::pair<std::size_t, std::size_t>> doomed;
    for (auto span : array_objects(s, "nodes"))
        if (top_field(slice(s, span), "id") == id) {
            doomed.push_back(span);
            break;
        }
    // Every edge touching id.<port>.
    for (auto span : array_objects(s, "edges")) {
        auto obj = slice(s, span);
        if (node_of(top_field(obj, "from")) == id || node_of(top_field(obj, "to")) == id)
            doomed.push_back(span);
    }
    std::sort(doomed.begin(), doomed.end());
    erase_spans(s, std::move(doomed));
}

void remove_edge(std::string& s, const std::string& from, const std::string& to) {
    for (auto span : array_objects(s, "edges")) {
        auto obj = slice(s, span);
        if (top_field(obj, "from") == from && top_field(obj, "to") == to) {
            erase_spans(s, {span});
            return;
        }
    }
}

// The smallest "<type>_N" no current node uses (count+1 collided with
// surviving ids after a deletion → duplicate ids → shared lift instances,
// remove_node deleting the wrong twin).
std::string fresh_id(const Graph& g, std::string_view type) {
    for (int k = 1;; ++k) {
        std::string id = std::string(type) + "_" + std::to_string(k);
        bool taken = false;
        for (const auto& n : g.nodes) taken |= (n.id == id);
        if (!taken) return id;
    }
}

// set_param: rewrite (or insert) "params":{...,"<port>":<value>,...} on the
// node object. v1 is scalar-only — that's every editable port the sliders
// touch. (The old editor re-serialized live node state; this writes the op
// declaratively so it survives the swap.)
void set_param(std::string& s, const std::string& id, const std::string& port, double v) {
    if (id.empty() || port.empty()) return;
    for (auto span : array_objects(s, "nodes")) {
        if (top_field(slice(s, span), "id") != id) continue;
        char num[32];
        std::snprintf(num, sizeof(num), "%g", v);
        std::string node{slice(s, span)};
        std::string pkey = "\"params\":{";
        auto pp = node.find(pkey);
        if (pp == std::string::npos) {
            // No params object: add one before the closing brace.
            node.insert(node.size() - 1, ",\"params\":{\"" + escape(port) + "\":" + num + "}");
        } else {
            std::string field = "\"" + escape(port) + "\":";
            auto fp = node.find(field, pp);
            // Only treat as present if inside this params object.
            auto params_end = node.find('}', pp);
            if (fp != std::string::npos && fp < params_end) {
                auto vs = fp + field.size();
                auto ve = vs;
                while (ve < node.size() && node[ve] != ',' && node[ve] != '}') ++ve;
                node.replace(vs, ve - vs, num);
            } else {
                std::string ins = "\"" + escape(port) + "\":" + num;
                bool empty = node[pp + pkey.size()] == '}';
                node.insert(pp + pkey.size(), empty ? ins : ins + ",");
            }
        }
        s.replace(span.first, span.second - span.first + 1, node);
        return;
    }
}

}  // namespace

std::string apply_edit_op(const std::string& op_json, const Graph& g) {
    // A whole-graph command passes through verbatim (old editor full edits).
    if (op_json.find("\"nodes\"") != std::string::npos &&
        op_json.find("\"op\"") == std::string::npos)
        return op_json;

    std::string s = serialize_graph(g);
    std::string op = str_field(op_json, "op");

    if (op == "add_node") {
        std::string type = str_field(op_json, "type");
        if (type.empty()) return s;
        std::string id = str_field(op_json, "id");
        if (id.empty()) id = fresh_id(g, type);
        insert_in_array(
            s, "nodes", "{\"id\":\"" + escape(id) + "\",\"type\":\"" + escape(type) + "\"}");
    } else if (op == "remove_node") {
        remove_node(s, str_field(op_json, "id"));
    } else if (op == "add_edge") {
        std::string from = str_field(op_json, "from");
        std::string to = str_field(op_json, "to");
        if (from.empty() || to.empty()) return s;
        insert_in_array(
            s, "edges", "{\"from\":\"" + escape(from) + "\",\"to\":\"" + escape(to) + "\"}");
    } else if (op == "remove_edge") {
        remove_edge(s, str_field(op_json, "from"), str_field(op_json, "to"));
    } else if (op == "set_param") {
        std::string_view vs = op_json;
        std::string vneedle = "\"value\":";
        auto vp = vs.find(vneedle);
        double val = (vp != std::string_view::npos)
                         ? std::strtod(vs.data() + vp + vneedle.size(), nullptr)
                         : 0.0;
        set_param(s, str_field(op_json, "id"), str_field(op_json, "port"), val);
    }
    return s;
}
