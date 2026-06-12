#!/usr/bin/env python3
"""compile_node.py — compile a node .cpp to .so and upload to a running peer.

--target android cross-compiles with the NDK for the headset;
--target host compiles with the local clang++ for the desktop app.
"""
import argparse
import os
import subprocess
import sys


def include_flags(repo_root: str, src_path: str) -> list[str]:
    dirs = [
        "components/sygaldry_endpoints",
        "components/param_registry",
        "components/eyeballs_node_abi",
        "components/gpu_texture",
        "components/tri_mesh",
    ]
    flags = []
    for d in dirs:
        flags += ["-I", os.path.join(repo_root, d)]
    flags += ["-I", os.path.dirname(os.path.abspath(src_path))]
    for var in ("BOOST_INCLUDE_DIR", "EIGEN_INCLUDE_DIR", "EIGEN3_INCLUDE_DIR"):
        if os.environ.get(var):
            flags += ["-I", os.environ[var]]
    return flags


def compile_node(src_path: str, target: str, ndk_root: str, peer_url: str) -> bool:
    if target == "android":
        cc = (
            f"{ndk_root}/toolchains/llvm/prebuilt/linux-x86_64/bin/"
            "aarch64-linux-android33-clang++"
        )
        # the app links libc++ statically; the plugin must carry its own
        libs = ["-lGLESv3", "-static-libstdc++"]
    else:
        cc = os.environ.get("CXX", "clang++")
        libs = ["-lGLESv2"]
    repo_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

    out = src_path.replace(".cpp", f".{target}.so")
    # -fno-gnu-unique: otherwise the descriptor's local static is bound
    # process-wide and hot-reloading a type silently keeps the OLD version.
    # (glibc-only concept: bionic has no STB_GNU_UNIQUE, and android clang
    # rejects the flag.)
    flags = [cc, "-shared", "-fPIC", "-std=c++20", "-O2", "-DSYGALDREYE_PLUGIN"]
    if target != "android":
        flags.append("-fno-gnu-unique")
    cmd = (flags + include_flags(repo_root, src_path) + [src_path, "-o", out] + libs)
    result = subprocess.run(cmd, capture_output=True, text=True)
    if result.returncode != 0:
        print("Compile error:", result.stderr, file=sys.stderr)
        return False
    print("Compiled:", out)
    if not peer_url:
        return True

    try:
        import requests  # type: ignore[import]
    except ImportError:
        print("requests not installed; install with: pip install requests", file=sys.stderr)
        return False

    with open(out, "rb") as f:
        r = requests.post(f"{peer_url}/plugins", data=f.read(), timeout=30)
    print("Upload:", r.text)
    return r.status_code == 200


if __name__ == "__main__":
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("src", help="Path to the .cpp file to compile and upload")
    p.add_argument("--target", choices=["android", "host"], default="android")
    p.add_argument("--ndk", default=os.environ.get("ANDROID_NDK_ROOT", ""),
                   help="ANDROID_NDK_ROOT path (android target)")
    p.add_argument("--peer", default="",
                   help="Peer base URL to upload to (e.g. http://127.0.0.1:8930); "
                        "empty = compile only")
    args = p.parse_args()
    if args.target == "android" and not args.ndk:
        p.error("--ndk required for android target (or set ANDROID_NDK_ROOT)")
    sys.exit(0 if compile_node(args.src, args.target, args.ndk, args.peer) else 1)
