# Sliders are touch-strips, should be knobs; display snaps back

Travis (Quest session), three intertwined defects in vr_editor sliders:
1. Whole track is interactable: any trigger-hold inside the (width ×
   0.03 × 0.05) box sets value from x — dragging vertically across
   stacked sliders edits all of them in passing.
2. Knob semantics wanted: grab must START on the knob (small radius),
   then value follows relative hand motion while held; nothing else
   reacts.
3. Display snap-back: SliderWidget.value is never initialized from the
   node's actual param (defaults to 0 at build; rebuilt after every
   edit), so the knob jumps to a wrong position post-edit even though
   the param kept the value. Fix: populate widget values from node
   params on (re)build.
Note: also applies to ui_slider node (graph-native) — same grab-region
critique; its drag state DOES persist (no snap-back there).
