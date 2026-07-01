- Constant nodes (one for each endpoint type) that just output constant values
- euler2quat node that takes 3 angles and outputs a quaternion rotation
- quat2mat node that takes a quaternion and outputs a rotation matrix
- or maybe it would be better to just have a "rotation" endpoint type?
- do we allow automatic conversion in the graph? I guess not?

## ABI v7 tidy (post values-map death)
The C descriptor struct still carries dead fields (push_textures,
maybe the whole set_* family once params go through a single channel).
Removing them = ABI v7: bump version, recompile plugins from source
(no binary backcompat needed — single user). Fold into phase D or do
immediately after.
