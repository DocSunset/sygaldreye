# Training humanoid robots to garden (brainstorm 2026-06-23)

Application note, not yet a work item.

Use the graph (and VR embodiment) to teach humanoid robots to garden —
VR teleoperation / demonstration as the data-collection front end, the
patch wiring perception → policy → actuation, and a sim/physics side
for practice before the real plot. The scanner pieces give the robot a
model of the garden; the speech loop lets you instruct it.

Connects: the physics sandbox ([[physics_sandbox]]) and scanner
([[scanner_3d]]) as the sim + perception substrate; another face of
turning research into a working artifact ([[research_application_gap]]).

Open questions: robot/sim interface (ROS? Isaac? direct), how VR
demonstrations map to training data / policies, and sim-to-real.
