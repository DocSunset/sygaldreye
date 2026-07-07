---
name: cpp26
description: >-
  Reference for C++26 static reflection and code generation — go here whenever you
  work on reflection, static metaprogramming, or generating code from type/function
  declarations: the `^^` reflection operator, `std::meta::info`, `members_of` /
  `parameters_of` / `identifier_of` / `type_of`, splicers `[: :]`, expansion
  statements (`template for`), annotations, or generating descriptors / codecs /
  bindings from node declarations. Grounds "one declaration → in-motion struct +
  canonical codec + descriptor + JSON projection" in what C++26 and GCC 16 actually
  ship. Consult before writing reflection code or deciding whether a codegen step
  is possible today.
---

# C++26 reflection & code-gen

The whole sygaldreye codegen thesis — declare a node once, derive its struct,
codec, descriptor, and JSON projection — rests on C++26 static reflection.
This is what's real **today** (GCC 16), the flags, and where the sharp edge is.

## Bottom line

- **You CAN, today (GCC 16):** reflect types and their members, reflect a
  function's parameters **including their names**, read annotations, iterate
  reflections with `template for`, and splice reflections back into code. That
  covers introspection-driven codegen: struct → descriptor → codec → JSON.
- **You CANNOT, today:** synthesize *new* code by injecting token sequences
  (P3294). Code injection is **not in C++26** and **not in GCC 16**. Generate
  code by combining reflection + expansion statements + splicers + templates,
  not by string/token injection.
- **Flags:** everything reflection needs `-std=c++26 -freflection`. Expansion
  statements (`template for`) need only `-std=c++26`. All C++26 support in GCC 16
  is **experimental**.

## The P3096 question (function parameter names) — settled

**Yes.** P3096R12 "Function Parameter Reflection" was **voted into C++26** at the
WG21 Sofia meeting (2025) and is **implemented in GCC 16** (under the P2996
umbrella, `-std=c++26 -freflection`). You can reflect a function's parameters and
read each parameter's identifier via `identifier_of` (with `has_identifier` /
`u8identifier_of`), plus `type_of`, `return_type_of`, `has_default_argument`,
`is_explicit_object_parameter`, `has_ellipsis_parameter`, `is_function_parameter`.
Caveat: names live on a *declaration*. A bare *function type* carries no parameter
names; reflect the function (declaration/definition) to get them, and an unnamed
parameter has no identifier (`has_identifier` is false).

## Papers (all verified via wg21.link)

| Paper | Title | Link | Gives us | C++26 | GCC 16 |
|---|---|---|---|---|---|
| P2996R13 | Reflection for C++26 | https://wg21.link/p2996 | `^^`, `std::meta::info`, `members_of`, `identifier_of`, `type_of`, splicers `[: :]` — the core | **In** | Yes — `-std=c++26 -freflection` (PR120775) |
| P3096R12 | Function Parameter Reflection | https://wg21.link/p3096 | reflect params + their **names** | **In** | Yes (with P2996) |
| P3394R4 | Annotations for Reflection | https://wg21.link/p3394 | attach `[[=...]]` metadata to entities, read it back via reflection | **In** | Yes |
| P1306R5 | Expansion statements (`template for`) | https://wg21.link/p1306 | compile-time loop over a reflection range — the codegen driver | **In** | Yes — `-std=c++26` (PR120776) |
| P3491R3 | `define_static_{string,object,array}` | https://wg21.link/p3491 | promote compile-time data to static storage (names, tables) | **In** | Yes (PR120783) |
| P3560R2 | Error Handling in Reflection | https://wg21.link/p3560 | structured errors from reflection metafunctions | **In** | Yes |
| P3294R2 | Code Injection with Token Sequences | https://wg21.link/p3294 | `^{...}` token seqs + `queue_injection` to synthesize new code | **NOT in** (EWG, in flight) | **No** |

## Core mechanics (verified against P2996R13)

- **Reflect:** `^^ X` — prefix operator, yields a `std::meta::info` value from a
  grammatical construct (type, namespace, member, function, expression).
- **The handle:** `std::meta::info` — one opaque type for every reflected entity.
- **Introspect (in `std::meta`, `consteval`):** `members_of`,
  `nonstatic_data_members_of`, `parameters_of`, `identifier_of`, `type_of`,
  `return_type_of`, `template_arguments_of`, `has_identifier`. (Confirm exact
  signatures against P2996 before relying on one.)
- **Splice:** `[: r :]` — turn an `info` back into a grammatical element.
  Flavors: expression, type, namespace, and addressed (`&[: r :]`) splicers.
- **Iterate for codegen:** `template for (constexpr auto m : members_of(^^T)) { ... }`
  — expansion statement; unrolls at compile time so each `m` can be spliced.
- **Annotate:** `[[=value]]` on a declaration (P3394), read back by reflecting the
  entity — the clean channel for descriptor hints on a node declaration.

Pattern for sygaldreye codegen: reflect the node struct (`^^Node`), `template for`
over `nonstatic_data_members_of`, read each member's `identifier_of` / `type_of`
(and any P3394 annotation), and splice to emit descriptor entries / codec calls /
JSON keys. No token injection needed — and none is available.

## When you need more, read these

- **P2996 core (the reference):** https://wg21.link/p2996 →
  https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p2996r13.html
- **GCC 16 C++26 status (flags, PRs, what shipped):**
  https://gcc.gnu.org/gcc-16/changes.html
- **Function parameter reflection:** https://wg21.link/p3096
- **Expansion statements:** https://wg21.link/p1306
- **Annotations:** https://wg21.link/p3394
- **Code injection (track for the future, not usable yet):** https://wg21.link/p3294
- Any wg21.link/pNNNN redirects to the latest revision — use it to re-verify status.
