#!/usr/bin/env python3
"""companion.py — discover eyeballs headset on LAN, mirror params, transcribe audio"""
import argparse
import io
import json
import re
import socket
import tempfile
import threading
import time

import requests
from flask import Flask, request as flask_request
from faster_whisper import WhisperModel
from zeroconf import ServiceBrowser, ServiceListener, Zeroconf

app = Flask(__name__)

# Shared state set by discovery thread
_headset_url: str | None = None
_whisper: WhisperModel | None = None


def _get_whisper() -> WhisperModel:
    global _whisper
    if _whisper is None:
        _whisper = WhisperModel("base", device="cpu", compute_type="int8")
    return _whisper


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


def _discovery_loop(host=None, port=8080):
    global _headset_url
    if host:
        _headset_url = params_url(host, port).rstrip("/params")
        address, hport = host, port
    else:
        print("Searching for eyeballs headset via mDNS...")
        address, hport = discover()
        if address:
            _headset_url = f"http://{address}:{hport}"
    if not address:
        print("No eyeballs service found on LAN.")
        return
    url = params_url(address, hport)
    print(f"Polling {url} every 2s (Ctrl-C to stop)")
    while True:
        try:
            resp = requests.get(url, timeout=5)
            print(json.dumps(json.loads(resp.text), indent=2))
        except Exception as e:  # noqa: BLE001
            print(f"Error: {e}")
        time.sleep(2)


def main():
    parser = argparse.ArgumentParser(description="Eyeballs VR headset companion")
    parser.add_argument("--set", metavar="KEY=VALUE", help="POST a param change")
    parser.add_argument("--host", help="Headset IP (skip mDNS discovery)")
    parser.add_argument("--port", type=int, default=8080, help="HTTP port (default 8080)")
    parser.add_argument("--flask-port", type=int, default=9090, help="Flask listen port (default 9090)")
    args = parser.parse_args()

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
