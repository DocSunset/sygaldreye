# WI-1: Port schema reader

Parse the static JSON string from `EyeballsNodeDescriptor::port_schema` into a typed struct.

## Deliverable

`components/port_schema_reader/port_schema_reader.hpp` + `.cpp` + `CMakeLists.txt` + `.test.cpp`

## Acceptance

- `parse_port_schema(nullptr)` returns empty `PortSchema`
- `parse_port_schema("{\"inputs\":[{\"name\":\"x\",\"kind\":\"scalar\"}],\"outputs\":[]}")` returns 1 input, 0 outputs
- `sh/build.sh` passes
