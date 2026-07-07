#!/usr/bin/env python3
"""freeze_graph.py — generate a static C++ frozen graph from runtime JSON"""
import argparse
import json
import sys

import requests


def snake_to_camel(s: str) -> str:
    return "".join(w.capitalize() for w in s.split("_"))


def freeze_graph(graph_json: dict, out_path: str) -> None:
    lines = [
        "// frozen_graph.cpp — do not edit, produced by freeze_graph.py",
        '#include "eyeballs_node_abi.hpp"',
        '#include "param_registry.hpp"',
    ]

    for node in graph_json["nodes"]:
        prov = node.get("provenance", {})
        kind = prov.get("kind", "local")
        if kind in ("local", "inline"):
            header = prov.get("header", f"components/{node['type']}/{node['type']}.hpp")
            lines.append(f'#include "{header}"')
        elif kind == "nix":
            print(f"WARNING: nix provenance not yet implemented for {node['id']}, skipping")
        elif kind == "git":
            print(f"WARNING: git provenance not yet implemented for {node['id']}, skipping")

    lines += [
        "",
        "struct FrozenGraph {",
        '    static consteval std::string_view name() { return "frozen_graph"; }',
        "    struct inputs {} inputs;",
        "",
    ]

    type_map: dict[str, str] = {}
    for node in graph_json["nodes"]:
        cpp_type = snake_to_camel(node["type"])
        type_map[node["id"]] = cpp_type
        lines.append(f"    {cpp_type} {node['id']}{{}};")

    lines += ["", "    void operator()(double time) {"]

    for node in graph_json["nodes"]:
        lines.append(f"        {node['id']}(time);")
        for edge in graph_json.get("edges", []):
            if edge["from"].split(".")[0] == node["id"]:
                from_node, from_port = edge["from"].split(".", 1)
                to_node, to_port = edge["to"].split(".", 1)
                lines.append(f"        // edge: {edge['from']} -> {edge['to']}")
                lines.append(
                    f"        // {to_node}.inputs.{to_port}.value"
                    f" = {from_node}.outputs.{from_port}.value;"
                )

    lines += ["    }", "};", "", "EYEBALLS_EXPORT_NODE(FrozenGraph)"]

    with open(out_path, "w") as f:
        f.write("\n".join(lines) + "\n")
    print(f"Written: {out_path}")


def main() -> int:
    p = argparse.ArgumentParser(description="Generate a frozen C++ graph from JSON")
    p.add_argument("--headset", default=None, help="http://ip:port — fetch graph from headset")
    p.add_argument("--json",    default=None, help="path to graph JSON file")
    p.add_argument("--out",     default="frozen_graph.cpp", help="output .cpp path")
    args = p.parse_args()

    if args.headset:
        r = requests.get(f"{args.headset}/graph", timeout=10)
        graph = r.json()
    elif args.json:
        with open(args.json) as f:
            graph = json.load(f)
    else:
        print("Provide --headset or --json", file=sys.stderr)
        return 1

    freeze_graph(graph, args.out)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
