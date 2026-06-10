# net_nodes

Owning package: `scene` (network)

First graph-native network bridge: udp_send / udp_recv pair values across
process boundaries by channel number. Loopback-only v1; see
planning/network_bridge.md for the slice-4 successor (proxy nodes over
WebSocket) — these nodes become a legacy convenience then.

## Ports
- udp_send inputs: in (scalar), channel, port. No outputs.
- udp_recv inputs: channel, port. Outputs: out (scalar).
- Sources: the network (recv drains all pending datagrams per tick).
- Destinations: the network (send fires one datagram per tick).
- Temporal couplings: last-value-wins on receive; ~1 frame end-to-end on
  loopback. Sockets are created lazily on first tick and rebound when the
  port input changes.
- Intended seams: `host` text param (cross-device) is the next planned
  extension; binary payloads for vectors after that.

## Requirements
- Never block the render thread (nonblocking sockets, drain loop).
- Datagram format "channel value" ASCII, one value per packet.

## Allowed dependencies
sygaldry_endpoints, POSIX sockets.
