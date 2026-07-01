// Copyright 2026 Travis West
// Structured edit ops (vr_editor_decomposition.md S2): the editor's
// whole-graph JSON splicing, moved out of vr_editor_interactions.cpp into one
// tested place. Each op is applied as a transform on the serialized graph.
#include <cstdio>
#include <string>
#include <string_view>

#include "signal_graph.hpp"

namespace {

std::string_view str_field(std::string_view json, std::string_view key) {
    std::string needle = "\"";
    needle += key;
    needle += "\":\"";
    auto p = json.find(needle);
    if (p == std::string_view::npos) return {};
    p += needle.size();
    auto end = json.find('"', p);
    if (end == std::string_view::npos) return {};
    return json.substr(p, end - p);
}

// Erase the JSON object containing `at` (plus one adjacent comma) from `s`.
void erase_object_at(std::string& s, std::size_t at) {
    auto obj_start = s.rfind('{', at);
    if (obj_start == std::string::npos) return;
    int depth = 0;
    std::size_t obj_end = obj_start;
    for (std::size_t i = obj_start; i < s.size(); ++i) {
        if (s[i] == '{')
            ++depth;
        else if (s[i] == '}') {
            if (--depth == 0) {
                obj_end = i;
                break;
            }
        }
    }
    std::size_t rs = obj_start;
    if (rs > 0 && s[rs - 1] == ',')
        --rs;
    else if (obj_end + 1 < s.size() && s[obj_end + 1] == ',')
        ++obj_end;
    s.erase(rs, obj_end - rs + 1);
}

// Insert `entry` just before the close of the named array, with a separator
// iff the array is non-empty. Returns false if the array isn't found.
bool insert_in_array(std::string& s, std::string_view array_key, const std::string& entry) {
    std::string needle = "\"";
    needle += array_key;
    needle += "\":";
    auto kp = s.find(needle);
    if (kp == std::string::npos) return false;
    auto arr_end = s.find(']', kp);
    if (arr_end == std::string::npos) return false;
    std::string sep = (arr_end > 0 && s[arr_end - 1] != '[') ? "," : "";
    s.insert(arr_end, sep + entry);
    return true;
}

void remove_node(std::string& s, std::string_view id) {
    std::string id_needle = "\"id\":\"";
    id_needle += id;
    id_needle += "\"";
    if (auto p = s.find(id_needle); p != std::string::npos) erase_object_at(s, p);
    // Every edge touching node.<port>.
    for (std::string_view dir : {"from", "to"}) {
        std::string needle = "\"";
        needle += dir;
        needle += "\":\"";
        needle += id;
        needle += ".";
        for (auto p = s.find(needle); p != std::string::npos; p = s.find(needle))
            erase_object_at(s, p);
    }
}

void remove_edge(std::string& s, std::string_view from, std::string_view to) {
    std::string needle = "\"from\":\"";
    needle += from;
    needle += "\",\"to\":\"";
    needle += to;
    if (auto p = s.find(needle); p != std::string::npos) erase_object_at(s, p);
}

// set_param: rewrite (or insert) "params":{...,"<port>":<value>,...} on the
// node object. v1 is scalar-only — that's every editable port the sliders
// touch. (The old editor re-serialized live node state; this writes the op
// declaratively so it survives the swap.)
void set_param(std::string& s, std::string_view id, std::string_view port, double v) {
    std::string id_needle = "\"id\":\"";
    id_needle += id;
    id_needle += "\"";
    auto idp = s.find(id_needle);
    if (idp == std::string::npos) return;
    auto obj_start = s.rfind('{', idp);
    if (obj_start == std::string::npos) return;
    int depth = 0;
    std::size_t obj_end = obj_start;
    for (std::size_t i = obj_start; i < s.size(); ++i) {
        if (s[i] == '{')
            ++depth;
        else if (s[i] == '}') {
            if (--depth == 0) {
                obj_end = i;
                break;
            }
        }
    }
    char num[32];
    std::snprintf(num, sizeof(num), "%g", v);
    std::string node = s.substr(obj_start, obj_end - obj_start + 1);
    std::string pkey = "\"params\":{";
    auto pp = node.find(pkey);
    if (pp == std::string::npos) {
        // No params object: add one before the closing brace.
        std::string add = ",\"params\":{\"" + std::string(port) + "\":" + num + "}";
        node.insert(node.size() - 1, add);
    } else {
        std::string field = "\"";
        field += port;
        field += "\":";
        auto fp = node.find(field, pp);
        // Only treat it as present if inside this params object (before its '}').
        auto params_end = node.find('}', pp);
        if (fp != std::string::npos && fp < params_end) {
            auto vs = fp + field.size();
            auto ve = vs;
            while (ve < node.size() && node[ve] != ',' && node[ve] != '}') ++ve;
            node.replace(vs, ve - vs, num);
        } else {
            std::string ins = "\"" + std::string(port) + "\":" + num;
            bool empty = node[pp + pkey.size()] == '}';
            node.insert(pp + pkey.size(), empty ? ins : ins + ",");
        }
    }
    s.replace(obj_start, obj_end - obj_start + 1, node);
}

}  // namespace

std::string apply_edit_op(const std::string& op_json, const Graph& g) {
    // A whole-graph command passes through verbatim (old editor full edits).
    if (op_json.find("\"nodes\"") != std::string::npos &&
        op_json.find("\"op\"") == std::string::npos)
        return op_json;

    std::string s = serialize_graph(g);
    std::string_view op = str_field(op_json, "op");

    if (op == "add_node") {
        std::string_view type = str_field(op_json, "type");
        if (type.empty()) return s;
        std::string_view id = str_field(op_json, "id");
        std::string entry = "{\"id\":\"";
        if (!id.empty())
            entry += std::string(id);
        else {
            entry += std::string(type);
            entry += "_";
            char b[24];
            std::snprintf(b, sizeof(b), "%ld", long(g.nodes.size()) + 1);
            entry += b;
        }
        entry += "\",\"type\":\"";
        entry += std::string(type);
        entry += "\"}";
        insert_in_array(s, "nodes", entry);
    } else if (op == "remove_node") {
        remove_node(s, str_field(op_json, "id"));
    } else if (op == "add_edge") {
        std::string_view from = str_field(op_json, "from");
        std::string_view to = str_field(op_json, "to");
        if (from.empty() || to.empty()) return s;
        std::string entry = "{\"from\":\"";
        entry += std::string(from);
        entry += "\",\"to\":\"";
        entry += std::string(to);
        entry += "\"}";
        insert_in_array(s, "edges", entry);
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
