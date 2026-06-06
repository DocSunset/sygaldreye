#!/usr/bin/env python3
"""compile_node.py — cross-compile a node .cpp to .so and upload to headset"""
import argparse
import os
import subprocess
import sys


def compile_node(src_path: str, ndk_root: str, headset_url: str) -> bool:
    cc = (
        f"{ndk_root}/toolchains/llvm/prebuilt/linux-x86_64/bin/"
        "aarch64-linux-android33-clang++"
    )
    repo_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    includes = [
        "-I", os.path.join(repo_root, "components/sygaldry_endpoints"),
        "-I", os.path.join(repo_root, "components/param_registry"),
        "-I", os.path.join(repo_root, "components/eyeballs_node_abi"),
    ]
    boost_inc = os.environ.get("BOOST_INCLUDE_DIR", "")
    if boost_inc:
        includes += ["-I", boost_inc]

    out = src_path.replace(".cpp", ".so")
    result = subprocess.run(
        [cc, "-shared", "-fPIC", "-std=c++20"] + includes + [src_path, "-o", out],
        capture_output=True,
        text=True,
    )
    if result.returncode != 0:
        print("Compile error:", result.stderr, file=sys.stderr)
        return False

    try:
        import requests  # type: ignore[import]
    except ImportError:
        print("requests not installed; install with: pip install requests", file=sys.stderr)
        return False

    with open(out, "rb") as f:
        r = requests.post(f"{headset_url}/plugins", data=f.read(), timeout=30)
    print("Upload:", r.text)
    return r.status_code == 200


if __name__ == "__main__":
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("src", help="Path to the .cpp file to compile and upload")
    p.add_argument("--ndk", default=os.environ.get("ANDROID_NDK_ROOT", ""),
                   help="ANDROID_NDK_ROOT path")
    p.add_argument("--headset", default="http://192.168.1.1:8080",
                   help="Headset HTTP base URL")
    args = p.parse_args()
    if not args.ndk:
        p.error("--ndk required (or set ANDROID_NDK_ROOT)")
    sys.exit(0 if compile_node(args.src, args.ndk, args.headset) else 1)
