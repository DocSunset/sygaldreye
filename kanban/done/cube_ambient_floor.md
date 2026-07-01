# cube node: faces away from light render pure black

CubeNode lambert shading has no ambient floor — with the default sun
pointing straight down (sun_light at noon phase), every side face is
black. Add small ambient term or wire material ambient input.
Seen in UI-panel demo screenshots 2026-06-10.
