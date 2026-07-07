# hello

Greets the user.

## Ports

**Outputs**
- Writes `"hello, world\n"` to the provided stream.

**Destinations**
- `std::ostream& out` — the stream receiving the greeting; must be open and writable.

**Intended seams**
- `std::ostream& out` parameter — substitute any `std::ostream` (e.g. `std::ostringstream`) for testing.

## Requirements

- `greet(out)` writes exactly `"hello, world\n"` to `out`.

## Allowed dependencies

*(none — standard library only)*

## Owning package

hello
