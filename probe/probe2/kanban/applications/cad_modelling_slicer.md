# 3D CAD / artistic modelling / sculpting + 3D printer slicer (brainstorm 2026-06-23)

Application note, not yet a work item.

Use the graph for spatial 3D authoring — parametric CAD, freeform
artistic modelling, and direct hand sculpting in-VR — with geometry as
wirable graph content. Then close the fabrication loop: a slicer node
turns models into printer toolpaths (G-code). Pairs with the 3D scanner
note (scan → edit/sculpt → print) — see [[scanner_3d]].

Open questions: geometry kernel (B-rep/parametric vs mesh vs SDF/voxel
for sculpting), how parametric history maps to the dataflow graph, and
slicer scope (own slicing vs export to an existing slicer).
