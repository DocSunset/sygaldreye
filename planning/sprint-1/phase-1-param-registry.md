# Phase 1: Param Registry

## Goal

Every tweakable value in every component is discoverable, named, range-annotated, and
serializable to/from JSON — with zero manual registration code. Components opt in by
declaring an `inputs` struct using sygaldry endpoint types; PFR does the rest.

## Deliverables

- Sygaldry as a git submodule (metadata helpers only: `string_literal`, `num_literal`,
  `range_`, `persistent`, `occasional`, endpoint templates `slider<>`, `toggle<>`,
  `text<>`, `bng<>`)
- Boost.PFR added to the Nix environment
- `param_registry` component: walks any component's `inputs` struct via PFR, accumulates
  `{path, read, write}` accessor triples
- JSON serialization: `to_json(component)` / `from_json(component, json_string)` generated
  automatically from PFR iteration + sygaldry type traits
- At least one existing component (`water_surface`) migrated to declare `inputs` as a
  sygaldry aggregate to prove the pattern

## Port Declaration Pattern

```cpp
// Before: scattered private member floats, manually passed as arguments
// After:
struct WaterSurface {
    struct inputs {
        slider<"wave height", "", float, 0.f, 20.f, 5.f>  wave_height;
        slider<"wave speed",  "", float, 0.1f, 5.f, 1.f>  wave_speed;
        slider<"choppiness",  "", float, 0.f, 1.f,  0.3f> choppiness;
        slider<"sun elevation","",float,-1.f, 1.f,  0.3f> sun_elevation;
    } inputs;
    // ...
};
```

PFR iterates `inputs`, detects `slider<>` / `toggle<>` members by concept, and generates:
- JSON key from `pfr::get_name<I>(inputs)` (C++ field name, snake_case)
- Human-readable label from `MemberType::name()` (sygaldry metadata)
- Range from `MemberType::min()` / `MemberType::max()` / `MemberType::init()`

## Key Design Decisions

**PFR `get_name` requires Clang ≥ 14 or GCC ≥ 13.** Verify the NDK's Clang version
supports it before committing to this approach. Fallback: add a `static consteval auto name()`
to the endpoint template (avendish style) and use that as the JSON key instead.

**`inputs` must be a simple aggregate** (no user constructors, no private members, no
base classes with data). This is enforced by the sygaldry endpoint type design and should
be kept as a hard rule.

**No outputs in Phase 1.** `outputs` structs are introduced in Phase 5 when the runtime
graph needs to wire them. Phase 1 only needs `inputs` for the param registry.

## Dependencies

None. This phase is self-contained.

## Migration path for existing components

Existing components retain their current private member layout internally. The `inputs`
struct is added as a public aggregate; component methods read from `inputs.foo.value`
instead of private members. This is a mechanical refactor, done incrementally one component
at a time.
