# -Werror rollout

"Treat all warnings as errors" is discipline-only — no -Werror anywhere
in the build. Add it to project components (not vendored code:
whisper.cpp, NDK gtest, mongoose) once each target compiles clean.
reports/audit_conformability_editor_arc.md §7.
