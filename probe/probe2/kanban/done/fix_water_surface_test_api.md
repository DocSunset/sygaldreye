# fix water_surface test API

`water_surface.test.cpp` calls `w.mesh_data()` but `WaterSurface` has no such method.
Tests fail to compile. Either expose `mesh_data()` on `WaterSurface` or rewrite the tests
to verify observable behaviour without accessing private mesh data.
