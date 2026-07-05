# grab_detector

Proximity-based grab/release detector. Matches hand positions against grab targets each frame.

## Ports

**Inputs**
- `hands`: current per-hand state (position, validity, grip button)
- `targets`: mutable grab target state (position, radius, grabbed flag)

**Outputs**
- mutates `targets[i].grabbed`, `grabbing_hand`, `grab_offset`

**Temporal couplings**
- Must be called once per frame with current state; grip transitions are inferred from current vs. target state, not stored history.

## Requirements

- A valid hand pressing grip grabs the closest ungrabbed target within its radius.
- A hand not pressing grip releases any target it holds.
- A hand holds at most one target at a time.
- An invalid hand neither grabs nor releases.

## Allowed dependencies

- `grab_target`

## Owning package

`scene`
