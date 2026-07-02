# serialize_graph / parse_graph don't escape node ids

The edit-op layer escapes ids/ports end-to-end (editor_layout::json_escape,
escape-aware readers in signal_graph_edit.cpp), but serialize_graph and
parse_graph (signal_graph_serial.cpp) emit/read ids raw — an id containing
a quote or backslash still can't survive a persist/reload round trip.

Fix: escape on emit, unescape on read, in signal_graph_serial.cpp; add a
round-trip test with `we"ird\1` as a node id.
