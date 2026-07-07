# golden frame — properties, not bytes

The PKG-4.3 gate (and every pixel witness after it) asserts PROPERTIES of
the rendered RGBA output. Determinism class: approximate — rasterization
varies across GPUs and drivers (Mesa on the host, Adreno on the Quest), so
BYTE EQUALITY IS NEVER THE TEST. A byte-golden image would be a lie that
passes on exactly one machine.

The fixture graph: `render_head` → `draw` fed by `mesh_from_spans` (one
triangle from a `positions` data default — v1 reads a list default, not a
wired span edge; see STRENGTHENINGS 2026-07-06) + `surface_flat`, plus one
STRAY draw carrying a second triangle but NOT wired into the head's chain.

1. Every background pixel equals the clear color exactly (the clear is a
   memset, not rasterization — exactness is legitimate there).
2. Foreground coverage (pixels not equal to the clear color) is within
   ±15% of the triangle's analytic area at the render size.
3. The color centroid of foreground pixels lies within a tolerance box
   (±3% of frame extent) of the triangle's analytic centroid.
4. A scripted `set_param` op between frames translates the triangle; the
   measured centroid moves in the predicted direction and magnitude
   (±20%).
5. The stray (unchained) draw contributes ZERO coverage: frames with and
   without the stray node have identical foreground pixel counts
   (PKG-4.2's chain law, restated in pixels).
6. No pixel channel is NaN-patterned garbage: the set of distinct colors
   in a flat-shaded frame is small (clear + one fill ± MSAA edge blends).

BLESSING (ch. 17, same protocol as golden-audio.md): when the first
triangle renders, the take goes to Travis as a PNG; he looks and blesses
by committing it as a capture. Thereafter regression compares against
these properties — never against the blessed image's bytes.

BLESSED 2026-07-06 by Travis ("a red equilateral triangle on a black
background"): `documentary/media/2026-07-06-first-triangle.png`
CID `bafkr4iczwgngogpivmep5ipdhica73cv5eqnbm3ofz5bvo7mfeaw2wpo4q`
(blake3-256, raw lane — `./syg hash`). The first light of the greenfield's
render pipeline: real headless GLES3 through graph → executor →
render_region. Regression from here: the property checks above.
