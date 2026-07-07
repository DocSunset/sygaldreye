#!/usr/bin/env python3
"""compile_frozen.py — freeze, cross-compile, and upload a static graph to the headset"""
import argparse
import os
import subprocess
import sys
import tempfile

import requests


def compile_frozen(headset_url: str, ndk_root: str, repo_root: str) -> bool:
    with tempfile.TemporaryDirectory() as tmp:
        frozen_cpp = os.path.join(tmp, "frozen_graph.cpp")
        frozen_so  = os.path.join(tmp, "frozen_graph.so")

        # Step 1: generate frozen_graph.cpp
        result = subprocess.run(
            [sys.executable, os.path.join(repo_root, "companion/freeze_graph.py"),
             "--headset", headset_url, "--out", frozen_cpp],
            capture_output=True, text=True,
        )
        if result.returncode != 0:
            print("freeze_graph failed:", result.stderr)
            return False
        print(result.stdout, end="")

        # Step 2: cross-compile
        cc = (
            f"{ndk_root}/toolchains/llvm/prebuilt/linux-x86_64"
            "/bin/aarch64-linux-android33-clang++"
        )
        includes = [
            "-I", f"{repo_root}/components/sygaldry_endpoints",
            "-I", f"{repo_root}/components/param_registry",
            "-I", f"{repo_root}/components/eyeballs_node_abi",
            "-I", f"{repo_root}/components/gpu_texture",
            "-I", f"{repo_root}/components/water_surface",
            "-I", f"{repo_root}/components/sky_dome",
        ]
        boost_inc = os.environ.get("BOOST_INCLUDE_DIR", "")
        eigen_inc = os.environ.get("EIGEN3_INCLUDE_DIR", "")
        if boost_inc:
            includes += ["-I", boost_inc]
        if eigen_inc:
            includes += ["-I", eigen_inc]

        r = subprocess.run(
            [cc, "-shared", "-fPIC", "-std=c++20"] + includes + [frozen_cpp, "-o", frozen_so],
            capture_output=True, text=True,
        )
        if r.returncode != 0:
            print("Compile failed:", r.stderr)
            return False

        # Step 3: upload
        with open(frozen_so, "rb") as f:
            resp = requests.post(f"{headset_url}/plugins", data=f.read(), timeout=30)
        print("Upload:", resp.text)
        return resp.status_code == 200


def main() -> int:
    p = argparse.ArgumentParser(description="Freeze, compile, and upload a static graph")
    p.add_argument("--headset", default="http://192.168.1.1:8080")
    p.add_argument("--ndk",     default=os.environ.get("ANDROID_NDK_ROOT", ""))
    p.add_argument("--repo",    default=os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    args = p.parse_args()

    if not args.ndk:
        print("Set ANDROID_NDK_ROOT or pass --ndk", file=sys.stderr)
        return 1

    return 0 if compile_frozen(args.headset, args.ndk, args.repo) else 1


if __name__ == "__main__":
    raise SystemExit(main())
