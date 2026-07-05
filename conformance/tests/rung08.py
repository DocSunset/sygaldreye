"""Rung 8 — stubs. Each entry: criterion id -> callable or None (pending).
Write the test when you reach this rung; the assertion is quoted from the book.
Never weaken a test to pass it — amend the book by ADR instead."""

TESTS = {
    # 05-compilation.md: A/B chime interpreted-vs-frozen: spectrogram diff roughly equals 0; block-time
    "FRZ-1.1": None,
    # 05-compilation.md: unfreeze(artifact) yields the source graph hash; re-freezing is a
    "FRZ-1.2": None,
    # 05-compilation.md: hello-cosine's block region reports tier 1; add a tmux node and
    "FRZ-2.1": None,
    # 05-compilation.md: arm-none-eabi build of frozen chime links clean (long before real
    "FRZ-3.1": None,
    # 05-compilation.md: Quest wires a graph to the remote freeze node; the signed .so
    "FRZ-4.1": None,
    # 07-capability-packages.md: a package can be omitted from a target at build time and the
    "PKG-1.1": None,
    # 07-capability-packages.md: full existing audio test suite green after the retrofit; region
    "PKG-2.1": None,
    # 07-capability-packages.md: on-device: editor operable in-headset with the xr executor owning
    "PKG-3.1": None,
    # 07-capability-packages.md: head_pose/controller data reaches the graph only through package
    "PKG-3.2": None,
    # 07-capability-packages.md: `grep -rn glDraw components/ | grep -v render_region` is empty.
    "PKG-4.1": None,
    # 07-capability-packages.md: a draw node not wired into the head's chain does not render.
    "PKG-4.2": None,
    # 07-capability-packages.md: Quest schedules a heavy analysis derivation; with the worker
    "PKG-5.1": None,
    # 07-capability-packages.md: two-peer test — consumer patch drives a cube from a provider lfo
    "PKG-6.1": None,
    # 07-capability-packages.md: the same hello-cosine compiles locally on host (audio present) and
    "PKG-7.1": None,
    # 07-capability-packages.md: hot-plugging a USB audio device on host splices the audio package
    "PKG-8.1": None,
}
