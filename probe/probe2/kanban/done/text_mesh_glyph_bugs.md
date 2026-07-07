# text_mesh glyph rendering bugs (host)

Observed in host editor screenshots (2026-06-10):
- "water_surface" renders as "wator_surfaco" — 'e' glyphs draw as 'o' in some
  positions (atlas UV off-by-one? kerning table?)
- Slider labels on node cards render as garbage glyphs ("E", "[")

Repro: sh/agent/launch.sh; camera.sh 0 0.1 0.6 0 -0.15; screenshot.sh
Suspect dejavu_sans metrics/atlas mismatch in text_mesh.
