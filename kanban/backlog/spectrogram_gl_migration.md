# Spectrogram texture → ImageTex boundary path

`spectrogram.cpp:51-54` owns its own GL texture (glGenTextures +
glTexImage2D) — a GL-owning node behind "render_region is the sole GL
boundary" (ADR-008; audit §5). Migrate: emit the rolling STFT columns
as a CPU image on the existing ImageTex uniform path; render_region
uploads and caches. Deletes the node's GL.
reports/audit_conformability_editor_arc.md §5.
