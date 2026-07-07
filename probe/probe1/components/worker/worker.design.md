# worker

The worker region (edge_executor.design.md step 4, v1): one background
thread per peer where jobs may block — `system()`, sleeps, HTTP. The
render region posts jobs instead of stalling; the worker tier is implied
by queue mappings rather than a declared rate (per the design's open
question).

## Ports

Library component.

- Inputs: `post(std::function<void()>)` from any thread; jobs run in
  posting order, never dropped.
- Outputs: none directly — jobs communicate results through shared state
  they capture (atomics / shared_ptr blocks owned by the posting node).
- Temporal couplings: jobs must capture state by value or via
  shared_ptr — a node instance may be destroyed (graph swap) while its
  job is queued.
- Intended seams: `shared()` is the peer's single worker; replace with
  per-region instances when the executor starts inferring regions.

## Requirements

- Posting never blocks beyond a mutex.
- FIFO execution; drains remaining jobs on destruction.

## Allowed dependencies

Standard library only.

## Owning package

scene (graph infrastructure)
