// Copyright 2025 Travis West
#pragma once
#include "sygaldry_endpoints.hpp"
#include <boost/pfr.hpp>
#include <boost/pfr/core_name.hpp>
#include <charconv>
#include <concepts>
#include <cstddef>
#include <cstdio>
#include <string>
#include <string_view>

template<typename F>
concept SliderField = requires(F f) {
    { F::name() } -> std::convertible_to<std::string_view>;
    { F::min()  } -> std::convertible_to<double>;
    { F::max()  } -> std::convertible_to<double>;
    { F::init() };
    f.value;
};

template<typename F>
concept ToggleField = requires(F f) {
    { F::name() } -> std::convertible_to<std::string_view>;
    { f.value   } -> std::convertible_to<bool>;
    requires !SliderField<F>;
};

template<typename F>
concept TextField = requires(F f) {
    { F::name() } -> std::convertible_to<std::string_view>;
    { f.value   } -> std::convertible_to<std::string>;
    requires !SliderField<F>;
};

// Event-rate port: fires for exactly one tick. Not a param (transient),
// flows through edges as a 0/1 scalar copy each tick.
template<typename F>
concept BangField = requires(F f) {
    { F::name() }    -> std::convertible_to<std::string_view>;
    { f.triggered }  -> std::convertible_to<bool>;
};

namespace detail {

template<typename T>
std::string value_to_json(const T& v) {
    if constexpr (std::is_same_v<T, bool>) {
        return v ? "true" : "false";
    } else if constexpr (std::is_floating_point_v<T>) {
        char buf[32];
        auto [ptr, ec] = std::to_chars(buf, buf + sizeof(buf), v);
        return std::string(buf, ptr);
    } else {
        char buf[32];
        auto [ptr, ec] = std::to_chars(buf, buf + sizeof(buf), v);
        return std::string(buf, ptr);
    }
}

template<typename T>
void set_from_sv(T& v, std::string_view s) {
    if constexpr (std::is_same_v<T, bool>) {
        v = (s == "true");
    } else if constexpr (std::is_same_v<T, float>) {
        std::sscanf(s.data(), "%f", &v);
    } else if constexpr (std::is_same_v<T, double>) {
        std::sscanf(s.data(), "%lf", &v);
    } else if constexpr (std::is_integral_v<T>) {
        std::from_chars(s.data(), s.data() + s.size(), v);
    }
}

} // namespace detail

// Serialize all inputs fields of node to a flat JSON object.
template<typename T>
std::string to_json(const T& node) {
    std::string out = "{";
    bool first = true;
    boost::pfr::for_each_field(
        node.inputs,
        [&]<std::size_t I>(const auto& field, std::integral_constant<std::size_t, I>) {
            constexpr auto key = boost::pfr::get_name<I, typename std::remove_cvref_t<decltype(node.inputs)>>();
            using F = std::remove_cvref_t<decltype(field)>;
            // Only value-bearing fields serialize; emitting a key with no
            // value (e.g. vector ports) produces invalid JSON.
            if constexpr (SliderField<F> || ToggleField<F> || TextField<F>) {
                if (!first) out += ',';
                first = false;
                out += '"';
                out += key;
                out += "\":";
                if constexpr (TextField<F>) {
                    out += '"';
                    for (char c : field.value) {  // minimal escape
                        if (c == '\n')      { out += "\\n"; }
                        else if (c == '\t') { out += "\\t"; }
                        else {
                            if (c == '"' || c == '\\') out += '\\';
                            out += c;
                        }
                    }
                    out += '"';
                } else {
                    out += detail::value_to_json(field.value);
                }
            }
        });
    out += '}';
    return out;
}

// Deserialize a flat JSON object into node inputs. Unknown keys are ignored.
template<typename T>
void from_json(T& node, std::string_view json) {
    // Minimal parser: expects {"key":value,...}
    auto pos = json.find('{');
    if (pos == std::string_view::npos) return;
    pos++;
    while (pos < json.size()) {
        // skip whitespace
        while (pos < json.size() && json[pos] != '"' && json[pos] != '}') ++pos;
        if (pos >= json.size() || json[pos] == '}') break;
        ++pos; // skip opening quote
        auto key_end = json.find('"', pos);
        if (key_end == std::string_view::npos) break;
        std::string_view key = json.substr(pos, key_end - pos);
        pos = key_end + 1;
        // skip colon
        while (pos < json.size() && json[pos] != ':') ++pos;
        ++pos;
        // skip whitespace
        while (pos < json.size() && json[pos] == ' ') ++pos;
        // read value: quoted strings may contain , } { — scan to the
        // closing quote; bare values end at , or }
        std::string_view val;
        if (pos < json.size() && json[pos] == '"') {
            auto q = pos + 1;
            while (q < json.size() &&
                   !(json[q] == '"' && json[q - 1] != '\\')) ++q;
            val = json.substr(pos + 1, q - pos - 1);
            pos = (q < json.size()) ? q + 1 : q;
        } else {
            auto val_end = json.find_first_of(",}", pos);
            if (val_end == std::string_view::npos) val_end = json.size();
            val = json.substr(pos, val_end - pos);
            pos = val_end;
        }

        // match key against inputs fields
        boost::pfr::for_each_field(
            node.inputs,
            [&]<std::size_t I>(auto& field, std::integral_constant<std::size_t, I>) {
                constexpr auto fname = boost::pfr::get_name<I, typename std::remove_cvref_t<decltype(node.inputs)>>();
                if (std::string_view(fname) == key) {
                    using F = std::remove_cvref_t<decltype(field)>;
                    if constexpr (TextField<F>) {
                        // JSON strings escape newlines; GLSL et al need them back
                        std::string out;
                        out.reserve(val.size());
                        for (std::size_t i = 0; i < val.size(); ++i) {
                            if (val[i] == '\\' && i + 1 < val.size()) {
                                char c = val[++i];
                                out += (c == 'n') ? '\n' : (c == 't') ? '\t' : c;
                            } else out += val[i];
                        }
                        field.value = std::move(out);
                    } else if constexpr (SliderField<F> || ToggleField<F>) {
                        detail::set_from_sv(field.value, val);
                    }
                }
            });
    }
}
