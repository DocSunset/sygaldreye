# syg.substrate.v0.0.0

*The peer-defining foundation. A process is a conforming Sygaldreye stage-0
peer iff it implements everything below identically — verified not by
declaration but by* agreement*: two peers agree iff they mint bit-identical
ids. This document is itself content; its hash names this substrate version.*

**Status: pre-beta.** Nothing here is stable. Any change of semantics bumps the
version; because this text is content-addressed, that bump is automatic — edit
a byte and every id derived from this substrate forks, by design.

## Conformance

Little-endian. Fixed sizes/aligns: `bool` 1/1, `float` 4/4, `double` 8/8,
`int64` align 8, `void*` ≥ 8, `hash64_fnv1a` 8/8, `atom_term` 24. A platform
violating these is not a peer — its mints fork every id.

## Identity — FNV-1a/64 (eternal)

`basis = 0xcbf29ce484222325`, `prime = 0x100000001b3`. `seed = basis`.
`step(h, b) = (h ⊕ b) · prime mod 2⁶⁴`; a fold applies `step` over bytes in
order. A `u64` absorbs **LSB-first** (8 steps); a digest as its 8 LSB-first
bytes. This algorithm is eternal — verifying v0 content requires it forever,
even after a successor hash arrives. *(The natural first sub-decree seam; kept
inline at v0.)*

## Node & preimage

A node is `(type, bytes)`. Its id `= fnv1a( type.digest ∥ data[0 .. size) )` —
the type as 8 LSB-first bytes, then the raw payload. **Size is not absorbed**
(one terminal variable field is unambiguous). Recompute = verify.

The id is a node's **most authoritative name** — immutable, self-verifying, the
one name a peer can neither forge nor let drift. A node may be *bound* to many
names (symbol-table entries, peer aliases, human labels), all mutable and
plural; it *has* exactly one id.

## GROUND — the zero digest `{0}`

By position: as a **type**, "decoder out of band — you already know"; as a
**name**, anonymous; as a **function body**, an undecreed/foreign transition
(no spec claimed). Every chain terminates here.

## Fiat roster — the unsayable 9

Minted before any store (the decree precedes the environment); each a string
typed GROUND. The type **formers** — `atom` (leaf), `structure` (product),
`variant` (sum), `sequence` (N of one type), `mutable_ptr`, `constant_ptr`,
`scope` (name chain) — plus `str` and `content`. Each is fiat because it is
unsayable from the others. `str` is fiat alone among leaves because the string
type cannot name itself with a string — the one fixed point.

## The canon — primitive types

Ordinary `atom` residents; the decree pins only the **names**, so peers mint
identical ids.

`nil` 0/1 · `b8` 1/1 · `i8 i16 i32 i64` · `u8 u16 u32 u64` · `f32` 4/4 ·
`f64` 8/8 · `utf8` 1/1 · `utf16` 2/2 · `utf32` 4/4 · `str` dynamic/1 ·
`ref` 8/8 · `hash64_fnv1a` 8/8.

Char mapping is **by C++ type, never platform signedness** (which would fork
ids): `char → utf8` (C++'s text-byte type), `signed char → i8`,
`unsigned char`/`std::byte` → `u8`, `char16_t → utf16`, `char32_t → utf32`;
`wchar_t` refused (variable width). A `str` is conceptually `sequence(utf8)`.

## Relations — decreed names

`construct` — mint-or-get an instance by type. *(later: `tick`, `erase`)*

## The .id law

A handle's `id` is the content-name of its **own** data (a store resident) or
`{}` (a grip or table row). A reference to another node lives in **data**,
interpreted by the type — never squeezed into `id`.

## Reference representation

A `ref` instance is its 8-byte target hash, carried **inline** in the data word
where the pointer admits it (§Conformance's `void* ≥ 8`); `id` stays `{}`.
Narrow-pointer peers (wasm32): table-owned heap via the `ref` constructor and
the ownership law — deferred.

---

*Deferred to later versions: `tick`/`erase` semantics; the layout fold
(structure/sequence term → size/align/offsets); `array(T, N)` (fixed-count
sibling of `sequence`); the small-data optimization policy; splitting this
substrate into finer sub-decrees.*
