# Add offline state machine tests for XrSessionObj::poll_events

Deferred from Phase 11. Requires extracting the transition logic into a pure function or building a stub XrSession handle. The transitions (READY→begin, STOPPING→end, EXITING→quit) need to run without a live OpenXR runtime. Implement as a dedicated work item after infrastructure for offline XR testing is established.
