# Remove dead RendererNode eye-override block in app.cpp

In `components/app/app.cpp` lines ~357–377, there is a block that iterates the
active graph to detect whether edges connect to `left_`/`right_` ports on the
`renderer` node, storing results in `left_eye_override`, `right_eye_override`,
and `renderer_node`. All three variables are immediately suppressed with
`(void)` casts. The feature was never completed.

Delete the block entirely: the loop, the three local variables, and the three
`(void)` lines. Also remove the `#include "renderer_node.hpp"` if it is only
pulled in for this block.
