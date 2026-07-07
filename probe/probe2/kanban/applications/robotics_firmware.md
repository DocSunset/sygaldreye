# Robotics, robot firmwares, 3D printers (brainstorm 2026-06-24)

Application note, not yet a work item.

Use the graph as the authoring/control layer for robotics broadly —
not just teleoperation/training ([[robot_gardening]]) but the lower
rungs too: robot firmware (motor control, kinematics, sensor fusion,
real-time loops as wirable nodes) and machine control like 3D printers
(toolpath execution, motion planning — the runtime end of the slicer in
[[cad_modelling_slicer]]).

The pitch: one medium from firmware up to behavior — wire a control
loop, watch it on the scope, tune it live, in-VR. The graph's
block/region execution model already thinks in real-time signal flow.

Open questions: real-time / hard-deadline execution on
microcontrollers vs the current Quest/host runtime, firmware
deployment target (does the graph compile down to MCU code?), and
safety/E-stop.
