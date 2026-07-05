# Chapter 15 — Time, concurrency, memory, faults (TCF)

*The promises that hold when threads, clocks, and failures collide.
Governing ADRs: 013, 015, 016, 020, 021, 022. Supersedes the probe's
incomplete concurrency notes.*

## Time

- Each executor owns a **monotonic clock** and publishes it as an output;
  time-dependent nodes wire to it (ADR-015). No node reads wall-clock;
  kernels receive dt or per-sample increments (AUT-1).
- Cadence is not typed: three disciplines forever, clocks open (ADR-020).
  Musical/transport time is a package (backlog: transport_package).
- Cross-peer time: pairwise offset  plus-or-minus  uncertainty published as demand-driven
  data; event timestamps translate through it; **slew, never step**.
- Latency-sensitive values are **time-parametric kinds** evaluated at the
  consumer's clock (ADR-022 posed/predicted).

## Concurrency and memory model

The global rule: **within a region, no concurrency exists** (one thread, one
cadence, appliers before process in topological order); **between regions,
only mappings carry data**, and each mapping states its guarantee:

| mapping | guarantee | mechanism shape |
|---|---|---|
| latch (value → clocked) | latest complete value, applied at the consumer's boundary; writer never blocks | atomic slot / seqlock |
| snapshot (clocked → value) | last completed period's value | atomic publish at period end |
| queue (event, cross-thread) | never drops, never duplicates, FIFO | MPSC; hosted: grows amortized; movement targets: fixed capacity, overflow = fault (ADR-016) |
| ring (stream, cross-thread) | in-order samples, bounded staleness | SPSC ring |
| net (cross-peer) | per ADR-022: value coalesced, event reliable-ordered, stream sequenced+jitter-buffered; latency reified on the mapping |
| z⁻¹ (cycle) | exactly one sample (ADR-013) | island interleave / loop-carried |
| probe | observation never perturbs region timing | demand-driven copy-out |

**Ownership.** The plan owns all in-motion values; instances own only their
state structs. True edges are borrows within one region's plan — legal
because same-thread. At swap, the retiring plan completes its in-flight tick
before release; state moves by route (EXE-5); queued events survive swap
(the queue belongs to the crown/arbiter, not the plan).

**Demand and dirtiness (ADR-015)** are region-local bookkeeping: dirty bits
propagate within the value region on the arbiter's thread at op-apply and
event delivery; clocked regions are dirty-exempt.

## Faults (ADR-016, mechanized)

- **Expected errors are values** on declared fault outputs (ch. 13).
- **Throws**: declared to record on fault output; undeclared to terminate the
  containment unit.
- **Traps** (SIGSEGV/FPE/ILL): async-signal-safe testimony write, then
  death; the parent (spawn-and-park) completes testimony and applies its
  wired policy — restart with intensity limits (R restarts / T seconds,
  exponential backoff), then permanent severance.
- **Severance semantics**: a severed producer's consumers behave as
  unwired — inlet defaults take over (audio default = silence; unreached
  draw = undrawn).
- **NaN/Inf**: boundary scans only (dac guard, mapping crossings); detection
  severs the upstream island.
- **Hangs**: blocking is the worker region's monopoly; elsewhere, stale
  heartbeat to containment-unit restart. Worker jobs carry deadlines.
- **Overruns**: two timestamps per region tick + per-island attribution;
  the executor's deadline ladder (report to mute costliest island  to 
  degrade per policy patch).
- **Quarantine**: fresh plugins realize in subprocess regions; promotion is
  trust policy (MSH-5). Fault handling is a capability package; a sealed
  movement carries none of it.

## Requirements

**TCF-1 (mapping guarantees).** Each row of the table has a stress test:
queue N events cross-thread (no loss/dup/reorder — LNG-3.1); latch
mid-block write invisibility (EXE-4.1); ring bounded staleness; net
reconnect discipline (PKG-6.1).
**TCF-2 (swap safety).** 10,000 random swaps under load: no torn values, no
lost events, in-flight tick always completes (extends EXE-5.1).
**TCF-3 (clock honesty).** No wall-clock syscalls from any node
(grep + runtime intercept); executor clocks are monotonic; offset
translation slews (max slew rate asserted).
**TCF-4 (fault matrix).** One test per fault class by region kind:
declared-throw, undeclared-throw, trap-in-quarantine, NaN at dac, hang in
frame (watchdog), overrun in block (ladder) — each ends in the documented
state with testimony written (extends ABI-3, SZ-5).
**TCF-5 (movement austerity).** A frozen movement build contains no fault
package, no heartbeats, no dirty bookkeeping — binary-size and symbol
audits prove absence.
