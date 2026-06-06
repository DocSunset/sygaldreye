#!/usr/bin/env python3
"""companion.py — discover eyeballs headset on LAN and mirror params"""
import argparse
import json
import time

import requests
from zeroconf import ServiceBrowser, ServiceListener, Zeroconf


class EyeballsListener(ServiceListener):
    def __init__(self):
        self.address = None
        self.port = None

    def add_service(self, zc: Zeroconf, type_: str, name: str) -> None:
        info = zc.get_service_info(type_, name)
        if info and info.addresses:
            import socket
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


def main():
    parser = argparse.ArgumentParser(description="Eyeballs VR headset companion")
    parser.add_argument("--set", metavar="KEY=VALUE", help="POST a param change")
    parser.add_argument("--host", help="Headset IP (skip mDNS discovery)")
    parser.add_argument("--port", type=int, default=8080, help="HTTP port (default 8080)")
    args = parser.parse_args()

    if args.host:
        address, port = args.host, args.port
    else:
        print("Searching for eyeballs headset via mDNS...")
        address, port = discover()
        if not address:
            print("No eyeballs service found on LAN.")
            return 1

    url = params_url(address, port)

    if args.set:
        if "=" not in args.set:
            print("--set requires KEY=VALUE format")
            return 1
        key, value = args.set.split("=", 1)
        # Try to parse value as number, else send as string
        try:
            parsed = float(value) if "." in value else int(value)
        except ValueError:
            parsed = value
        payload = json.dumps({key: parsed})
        resp = requests.post(url, data=payload, timeout=5)
        print(f"POST {url} => {resp.status_code}: {resp.text}")
        return 0

    print(f"Polling {url} every 2s (Ctrl-C to stop)")
    while True:
        try:
            resp = requests.get(url, timeout=5)
            print(json.dumps(json.loads(resp.text), indent=2))
        except Exception as e:  # noqa: BLE001
            print(f"Error: {e}")
        time.sleep(2)


if __name__ == "__main__":
    raise SystemExit(main())
