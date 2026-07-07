# Pointer ray uses grip pose, not aim pose

Travis (Quest session): the ray doesn't point where his hand points.
input.cpp binds /input/grip/pose; Touch's grip orientation is rotated
~60° from natural aim. Bind /input/aim/pose (second pose action) and use
it for ray/tip; keep grip for held-object transforms. C++ (input layer).
