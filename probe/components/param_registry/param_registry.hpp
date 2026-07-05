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

namespace detail {
inline void append_escaped(std::string& out, const std::string& v) {
    out += '"';
    for (char c : v) {
        if (c == '\n')      { out += "\\n"; }
        else if (c == '\t') { out += "\\t"; }
        else {
            if (c == '"' || c == '\\') out += '\\';
            out += c;
        }
    }
    out += '"';
}
} // namespace detail

// Serialize a node's persisted params to a flat JSON object.
// v6: persisted fields are normalled fallbacks and cv offset/slope —
// NEVER live edge values (serialize-mid-modulation captures defaults).
template<typename T>
std::string to_json(const T& node) {
    std::string out = "{";
    bool first = true;
    static_assert(HasEndpoints<T>, "params are v6 endpoints");
    boost::pfr::for_each_field(node.endpoints,
        [&]<std::size_t I>(const auto& field, std::integral_constant<std::size_t, I>) {
            constexpr auto key = boost::pfr::get_name<I,
                typename std::remove_cvref_t<decltype(node.endpoints)>>();
            using F = std::remove_cvref_t<decltype(field)>;
            auto kv = [&](std::string_view k) {
                if (!first) out += ',';
                first = false;
                out += '"'; out += k; out += "\":";
            };
            if constexpr (V6Normalled<F>) {
                using VT = typename F::value_type;
                if constexpr (std::is_same_v<VT, std::string>) {
                    kv(key);
                    detail::append_escaped(out, field.fallback);
                } else if constexpr (std::is_arithmetic_v<VT>) {
                    kv(key);
                    out += detail::value_to_json(field.fallback);
                }
            } else if constexpr (V6Cv<F>) {
                if constexpr (std::is_arithmetic_v<typename F::value_type>) {
                    kv(key);
                    out += detail::value_to_json(field.offset);
                    kv(std::string(key) + "_slope");
                    out += detail::value_to_json(field.slope);
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

        // match key against fields
        auto unescape = [](std::string_view v) {
            std::string out;
            out.reserve(v.size());
            for (std::size_t i = 0; i < v.size(); ++i) {
                if (v[i] == '\\' && i + 1 < v.size()) {
                    char c = v[++i];
                    out += (c == 'n') ? '\n' : (c == 't') ? '\t' : c;
                } else out += v[i];
            }
            return out;
        };
        if constexpr (HasEndpoints<T>) {
            boost::pfr::for_each_field(node.endpoints,
                [&]<std::size_t I>(auto& field, std::integral_constant<std::size_t, I>) {
                    constexpr auto fname = boost::pfr::get_name<I,
                        typename std::remove_cvref_t<decltype(node.endpoints)>>();
                    using F = std::remove_cvref_t<decltype(field)>;
                    if constexpr (V6Normalled<F>) {
                        if (std::string_view(fname) != key) return;
                        using VT = typename F::value_type;
                        if constexpr (std::is_same_v<VT, std::string>)
                            field.fallback = unescape(val);
                        else if constexpr (std::is_arithmetic_v<VT>)
                            detail::set_from_sv(field.fallback, val);
                    } else if constexpr (V6Cv<F>) {
                        if constexpr (std::is_arithmetic_v<typename F::value_type>) {
                            if (std::string_view(fname) == key)
                                detail::set_from_sv(field.offset, val);
                            else if (std::string(fname) + "_slope" == key)
                                detail::set_from_sv(field.slope, val);
                        }
                    }
                });
        }
    }
}
