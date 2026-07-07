#!/usr/bin/env python3
"""companion.py — discover eyeballs headset on LAN, mirror params, transcribe audio"""
import argparse
import io
import json
import os
import re
import socket
import subprocess
import sys
import tempfile
import threading
import time

import requests
import sseclient
from flask import Flask, request as flask_request
from faster_whisper import WhisperModel
from zeroconf import ServiceBrowser, ServiceListener, Zeroconf

app = Flask(__name__)

_claude_param_url = None   # e.g. http://127.0.0.1:8930/param — voice → claude node
_claude_seq = 0
_play_url = None           # e.g. http://<headset>:8080/play — spoken replies


@app.route("/tts", methods=["POST"])
def tts():
    """Speak text: synthesize with espeak-ng, save, and push to the player."""
    body = flask_request.get_json(force=True, silent=True) or {}
    text = (body.get("text") or "").strip()
    if not text:
        return json.dumps({"ok": False, "error": "no text"}), 400
    wav_path = "/tmp/tts_last.wav"
    subprocess.run(["espeak-ng", "-v", "en-us", "-s", "165", "-w", wav_path, text],
                   check=True)
    pushed = False
    print(f"tts handler sees play_url={_play_url!r}")
    if _play_url:
        try:
            with open(wav_path, "rb") as f:
                requests.post(_play_url, data=f.read(), timeout=5)
            pushed = True
        except Exception as e:  # noqa: BLE001
            print(f"tts push failed: {e}")
    print(f"tts: {len(text)} chars → {wav_path} (pushed={pushed})")
    return json.dumps({"ok": True, "pushed": pushed}), 200

# Shared state set by discovery thread
_headset_url: str | None = None
_whisper: WhisperModel | None = None


def _get_whisper() -> WhisperModel:
    global _whisper
    if _whisper is None:
        _whisper = WhisperModel("base", device="cpu", compute_type="int8")
    return _whisper


def _fetch_palette() -> list:
    if not _headset_url:
        return []
    try:
        resp = requests.get(f"{_headset_url}/palette", timeout=3)
        return json.loads(resp.text)
    except Exception as e:  # noqa: BLE001
        print(f"palette fetch failed: {e}")
        return []


def _post_graph(graph_json: str) -> None:
    if not _headset_url:
        return
    try:
        resp = requests.post(f"{_headset_url}/graph", data=graph_json, timeout=3)
        print(f"graph update: {resp.status_code} {resp.text}")
    except Exception as e:  # noqa: BLE001
        print(f"graph update failed: {e}")


def _get_graph() -> dict | None:
    if not _headset_url:
        return None
    try:
        resp = requests.get(f"{_headset_url}/graph", timeout=3)
        return json.loads(resp.text)
    except Exception as e:  # noqa: BLE001
        print(f"graph fetch failed: {e}")
        return None


def _llm_graph(text: str, palette: list) -> str | None:
    api_key = os.environ.get("ANTHROPIC_API_KEY")
    if not api_key:
        return None
    try:
        resp = requests.post(
            "https://api.anthropic.com/v1/messages",
            headers={
                "x-api-key": api_key,
                "anthropic-version": "2023-06-01",
                "content-type": "application/json",
            },
            data=json.dumps({
                "model": "claude-haiku-4-5",
                "max_tokens": 512,
                "system": f"You control a VR scene graph. Available nodes: {json.dumps(palette)}. "
                          "Respond with ONLY a JSON graph to POST to /graph. "
                          'Format: {"nodes":[{"id":"n1","type":"type_name","params":{...}}],"edges":[]}',
                "messages": [{"role": "user", "content": text}],
            }),
            timeout=10,
        )
        data = json.loads(resp.text)
        return data["content"][0]["text"].strip()
    except Exception as e:  # noqa: BLE001
        print(f"LLM call failed: {e}")
        return None


@app.route("/transcribe", methods=["POST"])
def transcribe():
    wav_bytes = flask_request.get_data()
    with tempfile.NamedTemporaryFile(suffix=".wav", delete=False) as f:
        f.write(wav_bytes)
        tmp_path = f.name

    model = _get_whisper()
    segments, _ = model.transcribe(tmp_path, language="en")
    text = " ".join(s.text.strip() for s in segments).strip()
    print(f"transcript: {text!r}")

    # Voice→Claude: deliver the transcript to the claude_tmux node as a
    # param edit. seq bumps so the node fires once per utterance.
    if text and _claude_param_url:
        global _claude_seq
        _claude_seq += 1
        try:
            requests.post(_claude_param_url, timeout=3, data=json.dumps(
                {"node": "claude", "params": {"message": text, "seq": _claude_seq}}))
            print(f"→ claude node (seq {_claude_seq})")
        except Exception as e:  # noqa: BLE001
            print(f"claude forward failed: {e}")
        return json.dumps({"text": text}), 200, {"Content-Type": "application/json"}

    # Parse "set <word> <word> to <number>"
    m = re.fullmatch(r"set\s+(\w+)\s+(\w+)\s+to\s+([\d.+-]+)", text, re.IGNORECASE)
    if m and _headset_url:
        key = f"{m.group(1)}_{m.group(2)}".lower()
        try:
            value = float(m.group(3)) if "." in m.group(3) else int(m.group(3))
        except ValueError:
            value = m.group(3)
        try:
            resp = requests.post(f"{_headset_url}/params", data=json.dumps({key: value}), timeout=3)
            print(f"param update {key}={value}: {resp.status_code}")
        except Exception as e:  # noqa: BLE001
            print(f"param update failed: {e}")
    elif _headset_url and os.environ.get("ANTHROPIC_API_KEY"):
        # Fall back to LLM-driven graph update
        palette = _fetch_palette()
        graph_json = _llm_graph(text, palette)
        if graph_json:
            print(f"LLM graph: {graph_json}")
            _post_graph(graph_json)

    return json.dumps({"text": text}), 200, {"Content-Type": "application/json"}


class EyeballsListener(ServiceListener):
    def __init__(self):
        self.address = None
        self.port = None

    def add_service(self, zc: Zeroconf, type_: str, name: str) -> None:
        info = zc.get_service_info(type_, name)
        if info and info.addresses:
            self.address = socket.inet_ntoa(info.addresses[0])
            self.port = info.port
            print(f"Discovered eyeballs at {self.address}:{self.port}")

    def remove_service(self, zc: Zeroconf, type_: str, name: str) -> None:
        print(f"Service removed: {name}")

    def update_service(self, zc: Zeroconf, type_: str, name: str) -> None:
        pass


def params_url(address: str, port: int) -> str:
    return f"http://{address}:{port}/params"


def discover(timeout: float = 10.0):
    zc = Zeroconf()
    listener = EyeballsListener()
    ServiceBrowser(zc, "_eyeballs._tcp.local.", listener)
    deadline = time.time() + timeout
    while time.time() < deadline:
        if listener.address:
            zc.close()
            return listener.address, listener.port
        time.sleep(0.2)
    zc.close()
    return None, None


def _subscribe_sse(base_url: str) -> bool:
    """Subscribe to SSE /events stream; returns False if connection fails."""
    try:
        resp = requests.get(f"{base_url}/events", stream=True, timeout=10)
        client = sseclient.SSEClient(resp)
        print(f"SSE connected to {base_url}/events")
        for event in client.events():
            if event.event == "params":
                print("params update:", json.dumps(json.loads(event.data), indent=2))
            elif event.event == "graph":
                g = json.loads(event.data)
                if g.get("nodes"):
                    print("graph update:", json.dumps(g, indent=2))
        return True
    except Exception as e:  # noqa: BLE001
        print(f"SSE disconnected: {e}")
        return False


def _poll_loop(base_url: str) -> None:
    """Fallback: poll /params and /graph every 2s."""
    print(f"Polling {base_url} every 2s (Ctrl-C to stop)")
    while True:
        try:
            resp = requests.get(f"{base_url}/params", timeout=5)
            print("params:", json.dumps(json.loads(resp.text), indent=2))
        except Exception as e:  # noqa: BLE001
            print(f"params error: {e}")
        try:
            resp = requests.get(f"{base_url}/graph", timeout=5)
            g = json.loads(resp.text)
            if g.get("nodes"):
                print("graph:", json.dumps(g, indent=2))
        except Exception as e:  # noqa: BLE001
            print(f"graph error: {e}")
        time.sleep(2)


def _discovery_loop(host=None, port=8080):
    global _headset_url
    if host:
        _headset_url = f"http://{host}:{port}"
        address, hport = host, port
    else:
        print("Searching for eyeballs headset via mDNS...")
        address, hport = discover()
        if address:
            _headset_url = f"http://{address}:{hport}"
    if not address:
        print("No eyeballs service found on LAN.")
        return
    base_url = f"http://{address}:{hport}"
    # Try SSE first; fall back to polling on disconnect.
    while True:
        connected = _subscribe_sse(base_url)
        if not connected:
            # SSE unavailable — use polling until interrupted.
            _poll_loop(base_url)
            break
        print("SSE stream ended; reconnecting in 2s…")
        time.sleep(2)


def main():
    parser = argparse.ArgumentParser(description="Eyeballs VR headset companion")
    parser.add_argument("--set", metavar="KEY=VALUE", help="POST a param change")
    parser.add_argument("--host", help="Headset IP (skip mDNS discovery)")
    parser.add_argument("--port", type=int, default=8080, help="HTTP port (default 8080)")
    parser.add_argument("--flask-port", type=int, default=9090, help="Flask listen port (default 9090)")
    parser.add_argument("--claude-param-url", default="http://127.0.0.1:8930/param",
                        help="host_app /param URL for voice→claude forwarding ('' disables)")
    parser.add_argument("--play-url", default="",
                        help="player /play URL for spoken replies (e.g. http://headset:8080/play)")
    parser.add_argument("--freeze", action="store_true",
                        help="Freeze current graph, compile, and upload to headset, then exit")
    args = parser.parse_args()
    global _claude_param_url, _play_url
    _claude_param_url = args.claude_param_url or None
    _play_url = args.play_url or None
    print(f"voice: claude_param_url={_claude_param_url} play_url={_play_url}")

    if args.freeze:
        host = args.host or "127.0.0.1"
        headset_url = f"http://{host}:{args.port}"
        repo_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
        result = subprocess.run(
            [sys.executable, os.path.join(repo_root, "companion/compile_frozen.py"),
             "--headset", headset_url,
             "--repo", repo_root],
        )
        return result.returncode

    if args.set:
        if args.host:
            address, port = args.host, args.port
        else:
            print("Searching for eyeballs headset via mDNS...")
            address, port = discover()
            if not address:
                print("No eyeballs service found on LAN.")
                return 1
        if "=" not in args.set:
            print("--set requires KEY=VALUE format")
            return 1
        key, value = args.set.split("=", 1)
        try:
            parsed = float(value) if "." in value else int(value)
        except ValueError:
            parsed = value
        payload = json.dumps({key: parsed})
        resp = requests.post(params_url(address, port), data=payload, timeout=5)
        print(f"POST => {resp.status_code}: {resp.text}")
        return 0

    # Start Flask server in a background thread
    flask_thread = threading.Thread(
        target=lambda: app.run(host="0.0.0.0", port=args.flask_port, use_reloader=False),
        daemon=True)
    flask_thread.start()
    print(f"Transcription server listening on port {args.flask_port}")

    _discovery_loop(host=args.host, port=args.port)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
