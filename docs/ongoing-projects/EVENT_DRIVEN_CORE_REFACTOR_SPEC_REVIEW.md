# Adversarial Review: Event-Driven Core Refactor Specification

**Reviewed document:** [`EVENT_DRIVEN_CORE_REFACTOR_SPEC.md`](EVENT_DRIVEN_CORE_REFACTOR_SPEC.md) v0.4
**Reviewed implementation:** `src/game_scheduler.c`, `src/game_scheduler.h`,
`unittests/CuTest/test_game_scheduler.c` at branch `event-driven-core-refactor`
**Review date:** 2026-08-30
**Reviewer posture:** adversarial - the goal is to find what the specification
gets wrong, leaves undefined, or claims without evidence. Nothing below is a
recommendation to abandon the project.

## 0. Summary Judgment

This is a strong specification. It is source-grounded, its baseline claims about
the current code are accurate where I checked them, and the Phase 1 core it
describes actually exists, compiles, and is genuinely inert.

It is not yet ready to be stamped version 1.0. The material problems are:

1. The published public scheduling contract (Section 10) and the conceptual
   event record (Section 9.1) both include an owner model that the accepted
   Phase 1 implementation does not have and cannot be given without changing
   the event record, the cancel paths, and the admission path.
2. Several normative requirements - observability (Section 18), filtered
   inspection (Section 10), per-owner admission limits (Section 15.1) - are
   never assigned to any migration phase in Section 23.
3. The cascade algorithm in Section 8.3 is self-contradictory and does not
   describe the shipped implementation.
4. The monotonic-clock mandate in Section 7.1 conflicts with the existing
   `gettimeofday`-paced main loop, and no phase owns fixing it.
5. Copyover is listed in Phase 3's gate but the actual `execl` hazard for a
   `libevent` `event_base` is never stated.
6. Phase 6's rollback is not a rollback.
7. Acceptance criteria in Section 30 are largely unfalsifiable because the
   Phase 0 numeric baseline that was supposed to make them measurable was
   never captured, and the specification proceeded to Phase 1 anyway.

Findings are ordered by severity. Each names the section it applies to and,
where relevant, the source that contradicts it.

---

## 1. Blocking Findings

### F1. The public contract promises an owner model Phase 1 does not implement

Section 9.1 lists `owner_handle (optional)` and `owner-index linkage` as fields
of the conceptual event record. Section 10 lists `cancel_owner(owner) -> count`
in the public scheduling contract. Section 15.1 requires "Optional per-owner and
owner/type capacity". Section 25 requires that "Owner destruction does not scan
the global event population."

`src/game_scheduler.h` contains no owner concept at all. There is no
`owner_kind`, no `runtime_id`, no `generation`, no `game_scheduler_cancel_owner`,
and no owner index. `game_event_type_config` has `max_events` but no per-owner
limit. `game_event_snapshot` exposes wheel level, slot, and insertion sequence,
but no owner.

Section 23's Phase 1 deliverable list quietly omits owner cancellation, and
Section 23's implementation record for 2026-08-29 does not mention the omission.
The result is that a reader of Sections 9 through 15 will believe the accepted
core has an owner model, and a reader of the header will find it does not.

This matters beyond documentation accuracy. Section 8.1 states "Future geometry
changes ... must preserve the private API contract," and Section 23 Phase 1's
rollback claims the core "can be removed without affecting the live queue."
Retrofitting an owner handle later is not an additive change: it adds fields to
the event record, adds a second index that every admission and every terminal
transition must maintain, adds a new failure mode to admission (per-owner
capacity), and changes the cancellation surface. That is a rewrite of the
lifecycle code, and it lands in Phase 5, after Phases 2, 3, and 4 have already
built on the shape it changes.

**Required:** either (a) restate Section 10 as an explicitly phased contract,
marking `cancel_owner`, per-owner capacity, and owner linkage as Phase 5
additions with a stated expectation that the Phase 1 event record and cancel
path will change; or (b) move the owner index into Phase 1 before the Phase 1
gate closes. Option (b) is safer, because `cancel_owner` is the single hardest
guarantee in the whole design - "safe during dispatch" (Section 11.2) plus
"O(1) expected" (Section 25) plus "cleanup exactly once" (Section 11.2) is
exactly the interaction that the current `mud_event.c` gets wrong today, and
deferring it to Phase 5 defers the riskiest code past three integration phases.

### F2. Section 8.3's cascade ordering is self-contradictory and does not match the implementation

Section 8.3 reads:

> 2. If L0 wrapped, detach the current L1 slot and reinsert its events ...
> 3. If L1 also wrapped, cascade from L2 before cascading L1.

Step 2 cascades L1. Step 3 then says L2 must be cascaded *before* L1. Taken as
written, the two steps prescribe opposite orders for the same tick.

`advance_one_tick()` in `src/game_scheduler.c:627` implements neither literally.
It scans upward for the lowest level whose slot index is non-zero, cascades that
level first, and then cascades slot 0 of each lower level walking back down. On
a full wrap of every level it cascades from the highest level downward. That
behavior is correct - coarse-to-fine is the only ordering that avoids stranding
an event one level too high for a tick - but it is a third description, and it
is the one that is actually shipping.

**Required:** replace steps 2 through 4 with the coarse-to-fine rule the code
implements, and state the invariant explicitly: after cascading, no event whose
remaining time is below a level's slot width may still be stored at that level.

### F3. Section 7.1 mandates a monotonic clock; the main loop it must live in uses wall-clock

Section 7.1 is unambiguous: "Runtime deadlines MUST use a monotonic clock ...
System wall-clock adjustments MUST NOT make an event run early, late, or twice."

`src/comm.c` paces the game loop with `gettimeofday()` at lines 1055, 1114,
1160, 1240, 1266, and 1273. That is `CLOCK_REALTIME`. An NTP step or a manual
clock change today perturbs pulse pacing directly.

Section 23 never assigns anyone the job of fixing this. Phase 2 explicitly
continues "driving the scheduler from the existing heartbeat." Phase 3 promises
to "Invoke the existing pulse/heartbeat path from a 100 ms compatibility timer
so gameplay timing remains unchanged." Phase 4 arms a reactor timer from
`game_scheduler_next_deadline()`. In none of these does the tick source become
monotonic, and Phase 11 only removes things.

The scheduler core is clean here - it takes an injected
`game_scheduler_tick_source` and never reads a clock itself - so the defect is
entirely in the specification's silence about who supplies a monotonic tick and
when.

**Required:** add an explicit deliverable, at the latest in Phase 3, that the
production tick source is `clock_gettime(CLOCK_MONOTONIC)` (or
`CLOCK_MONOTONIC_COARSE`), and state the behavior on the platforms where
`src/comm.c:244` currently simulates `gettimeofday`. Also state what happens to
`GAME_SCHEDULER_CLOCK_REVERSED`, which the implementation can already return but
which Section 10's error list does not mention.

### F4. Copyover and `libevent` are on a collision course that the specification never names

Phase 3's gate includes "copyover" and Section 24.4 requires that "Shutdown,
signals, copyover, and restart preserve current operational behavior." That is
the entirety of the treatment.

LuminariMUD's copyover is an `execl()` of the same binary with descriptors
deliberately kept open across the exec, recovered by `copyover_recover()` at
`src/comm.c:553`. An `event_base` does not survive `exec`. Its epoll or kqueue
descriptor is a kernel object owned by a process image that ceases to exist.
Worse, backends may install `FD_CLOEXEC` or alter `O_NONBLOCK` on registered
descriptors, which is precisely the state copyover depends on. `event_reinit()`
exists for `fork`, not for `exec`, and does not apply here.

Separately, Section 6.1 says the reactor owns "Signal integration needed for
clean shutdown." The current signal setup at `src/comm.c:4293` installs
`SIGVTALRM` on `ITIMER_VIRTUAL` for `checkpointing()`, plus `SIGUSR1`,
`SIGUSR2`, `SIGHUP`, `SIGCHLD`, `SIGINT`, `SIGTERM`, and ignores `SIGPIPE` and
`SIGALRM`. `libevent` signal events install their own handlers and use a
socketpair; mixing them with the existing `my_signal()` installs is a real
conflict, and the interval timer saved in `checkpoint_timer_before_copyover`
(`src/comm.c:169`) is another piece of pre-exec state with its own ordering
requirement.

**Required:** Phase 3 must carry explicit deliverables for (1) tearing the
`event_base` down before `execl` and rebuilding it in the recovered process,
(2) an audit that no descriptor flag the reactor sets breaks copyover recovery,
and (3) a decision on which signals the reactor owns versus which stay on
`my_signal`. The gate should require a copyover under both drivers with live
descriptors attached, not merely "copyover" as a checklist word.

### F5. Phase 6's rollback is not a rollback

Phase 6's deliverables include "Remove runtime initialization and heartbeat
processing for `src/pubsub/`" and "Remove or separately re-specify its
player/staff commands and wilderness metadata; update help, documentation,
database setup, and rename handling."

Phase 6's rollback reads: "Retain the old pub/sub feature behind a boot-time
migration boundary until its commands and data obligations are reviewed."

You cannot roll back to a subsystem whose initialization, commands, help entries,
and rename hooks the same phase deleted. Section 23's opening rule - each phase
must be "reversible at each release gate" (Principle 10) - is violated here, and
this is the only phase where it is.

The footprint is not small. `pubsub` appears in 17 source files, registers three
commands in `src/interpreter.c:3195` (`pubsub`, `pubsubtopic`, `pubsubqueue`),
has 9 references in `lib/text/help/help.hlp`, and owns schema in
`sql/components/pubsub_v3_schema.sql`, `sql/master_schema.sql`, and
`lib/pubsub_v3_schema.sql`. Section 20.4 lists the retirement obligations
correctly but attaches no inventory, so no reviewer can tell whether the list is
complete.

**Required:** split Phase 6. Phase 6a introduces the typed domain-event core and
leaves pub/sub running but unused - that phase is genuinely reversible. Phase 6b
retires pub/sub, and its rollback is honestly stated as "revert the retirement
commit," which is a different and weaker guarantee than the boot-time gates used
elsewhere. Add the concrete inventory above to Section 20.4 so the retirement is
auditable.

---

## 2. Significant Findings

### F6. Normative requirements with no owning phase

Section 18 lists thirteen required metrics. Section 10 requires
`inspect(filter)`. Section 15.1 requires per-owner and per-owner/type capacity.
Section 24 requires several test families.

Searching Section 23 for where these land: Phase 1's deliverables do not include
them. Phase 4 mentions comparing "PERFMON behavior to Phase 0" but adds no
scheduler metrics. Phases 5 through 11 never mention them.

What the implementation actually exposes is `struct game_scheduler_stats`:
current tick, event count, ready count, overflow count, registered type count,
and five lifetime totals. Against Section 18 that is missing counts by event
type and owner kind, counts by wheel level, oldest overdue age, stale-owner and
rejected and skipped and coalesced totals, cascade count and largest cascade,
per-type callback timing and slow-handler samples, admission failures by reason,
and cross-thread ingress depth. `game_scheduler_inspect()` takes one event ID -
there is no filter by type, owner, deadline range, or state.

Some of that information does exist per-dispatch in
`game_scheduler_dispatch_report` (late callbacks, missed and skipped
occurrences, budget exhaustion flags, `used_large_advance`), which is good
design, but it is per-call and not accumulated.

**Required:** add an explicit observability deliverable to a named phase. Phase 4
is the natural home since it is the first phase where the scheduler carries real
production load, but Section 18 currently reads as a Phase 1 obligation and the
Phase 1 gate is being declared nearly met without it.

### F7. Section 30's acceptance criteria are unfalsifiable, and Phase 0's gate was skipped

Section 25 opens with "Numeric thresholds will be frozen after Phase 0
measurement." Section 13.3 says "Phase 0 records the current event callback
distribution and network-loop latency before numeric acceptance thresholds are
frozen." Phase 0's gate is "Approved specification and reproducible baseline
report."

Section 4.8 documents what was actually captured: a clean build, passing test
suites, a database migration check, and a bounded `autorun.sh` smoke test. Those
are correctness and environment checks. Not one of them is a queue-depth
histogram, a delay distribution, a callback-rate measurement, a slow-handler
census, or a heartbeat/network latency profile under representative load. The
specification acknowledges the gap honestly in the Phase 1 implementation
record - "not a substitute for the still-missing representative Phase 0
production delay trace" - but the project proceeded through Phase 1 anyway.

The consequence propagates. Section 30 then asks reviewers to accept criteria
like "Dispatch budgets preserve network responsiveness under load" and "no
unbounded latency regression" with no number attached to either. Section 15.1
calls the existing 10,000-event cap (`src/dgscript/dg_event.h:80`) "a baseline
to measure, not an assumed final value," and the implementation duly picked
10,000 as `GAME_SCHEDULER_DEFAULT_MAX_EVENTS` without the measurement.

**Required:** either capture the Phase 0 trace before closing the Phase 1 gate,
or amend Section 23 to state plainly that Phase 0's measurement gate is deferred
to Phase 4 and that every numeric threshold in the document is provisional until
then. The second option is legitimate; leaving Phase 0 nominally "complete" while
its defining deliverable is missing is not.

### F8. Section 6's architecture diagram has a node-ID collision

The Mermaid graph in Section 6 defines `C[Command interpreter]` and then
`D --> C[Combat encounters]`. Mermaid treats these as one node. The rendered
diagram silently merges the command interpreter and the combat encounter
subsystem into a single box - and it will render with whichever label was parsed
last, so the diagram shows the dispatcher feeding the command interpreter.

That is exactly the architecture Section 6.2 spends four paragraphs forbidding:
"Socket readiness does not execute game commands directly." The one diagram a
reviewer looks at first asserts the opposite of the text.

`L` is also reused as `L[Game logic]` while `D --> L` and `T --> S` cross the
domain-event boundary in ways the prose describes more precisely than the graph.

**Required:** rename the combat node (`E`, `K`, anything free) and re-render.
While there, verify the graph against Section 6.2 - the interpreter should be
reachable only from the descriptor input queue.

### F9. Discord and I3 are modeled as cross-thread producers but are select-set sockets today

Section 16 places I3, Discord, AI services, and database workers behind a
"bounded, thread-safe ingress queue" that wakes the reactor. Section 6.1 mentions
"Bounded wakeups for submissions from I3, Discord, AI, database, or future
worker threads."

In `src/comm.c:1131` and `src/comm.c:1139`, `discord_bridge->server_socket` and
`discord_bridge->client_socket` are folded into `maxdesc` and registered
directly in the `select()` set alongside player descriptors. They are sockets in
the main loop, not cross-thread submissions.

Phase 3's deliverables cover "listener readiness, descriptor read/write
readiness, required signals, and bounded cross-thread wakeups." Non-player
sockets that are neither the listener nor a player descriptor fall through that
list.

**Required:** state in Phase 3 which non-descriptor sockets the reactor must
adopt, and reconcile Section 16's thread model with the parts of I3 and Discord
that are in-loop sockets rather than worker threads. If the intent is to convert
them, that is a deliverable; if the intent is to register them as ordinary
reactor read events, say so.

### F10. Phase ordering places the reactor rewrite ahead of the ownership safety work

Phases run: 2 legacy adapter, 3 libevent reactor, 4 bridge and hardening, 5 MUD
event ownership adapter.

Section 4.3 identifies the actual danger in the current system: `struct
mud_event_data` stores an untyped `void *pStruct`, and "Character and object
safety relies heavily on canceling attached events before their owners are
freed." Section 12 is the fix. It is Phase 5.

So the plan replaces the entire I/O substrate - the highest blast-radius change
in the project, per its own risk table - while the use-after-free exposure it
inherits is still unaddressed, and while Phase 4 is simultaneously moving all
timed-event storage onto the new backend. If a crash appears during Phase 4
hardening, the differential diagnosis spans a new reactor, a new wheel, and the
legacy untyped owner pointers, all at once.

Phases 2 and 5 are both facade work over the same scheduler and share most of
their risk surface. Running 5 immediately after 2 would let Phase 3 and 4 begin
from a typed, generation-aware ownership base.

**Required:** justify the current order explicitly in Section 23, or reorder.
This is a judgment call rather than an error, but the specification currently
presents the order as self-evident and it is not.

### F11. Section 10's error list is incomplete against the implemented status enum

Section 10 says the API "returns structured errors for invalid type, invalid
owner, capacity, invalid deadline, and shutdown state."

`enum game_scheduler_status` in `src/game_scheduler.h` defines thirteen values.
Beyond the five named: `INVALID_ARGUMENT`, `INVALID_PAYLOAD`, `CLOCK_REVERSED`,
`TYPE_CAPACITY_REACHED`, `ALLOCATION_FAILED`, `NOT_FOUND`, `BUSY`, and
`ID_EXHAUSTED`. Conversely `INVALID_OWNER`, which the specification names, does
not exist.

Three of these are semantically interesting and undocumented:

- `GAME_SCHEDULER_BUSY` is returned from four sites, including
  `src/game_scheduler.c:1395` inside `advance`. Reentrant `advance` is a real
  failure mode with real consequences for a caller in the main loop, and the
  specification says nothing about it.
- `GAME_SCHEDULER_ID_EXHAUSTED` at `src/game_scheduler.c:1209` is the concrete
  handling of Section 24.1's "Event ID and insertion-sequence wrap defenses."
  Section 10 does not say what a caller should do when a 64-bit ID space is
  reported exhausted, and Section 15.2 does not classify it.
- `GAME_SCHEDULER_CLOCK_REVERSED` at `src/game_scheduler.c:1403` is the
  observable half of F3 and appears nowhere in the specification.

**Required:** make Section 10's error list normative and complete, and add
`BUSY`, `ID_EXHAUSTED`, and `CLOCK_REVERSED` to Section 15.2's failure policy
table with a stated caller obligation for each.

---

## 3. Moderate Findings

### F12. "Bounded" large advance is asserted, not bounded

Section 8.4 requires "an efficient bounded advance path" and forbids "one full
callback pass per missed tick."

The implementation honors the intent. `GAME_SCHEDULER_LARGE_ADVANCE_TICKS` is
`1 << 12` (4,096 ticks, roughly 6.8 minutes) at `src/game_scheduler.c:14`, and
`advance_large()` detaches all five levels times 64 slots and re-places every
event by exact deadline.

But the specification states no threshold and no cost. `advance_large` is
O(levels x slots + live events) - 320 slot heads plus a full re-placement of
every scheduled event in the process. On a heavily loaded server near the 10,000
event cap, that is a 10,000-event re-placement inside one main-loop iteration,
which is a latency spike, not a bound. The dispatch budget in Section 13.3
governs callbacks, not re-placement.

**Required:** state the threshold in Section 8.4, state the cost class, and say
whether re-placement is itself budgeted. If it is not budgeted, say why a
10,000-event re-placement is acceptable at the moment the server is already
recovering from a stall.

### F13. Section 13.2's reentrancy rule and Section 10's normalization rule are stated in incompatible vocabularies

Section 10: "A zero or past relative deadline is normalized according to
admission policy; it must not recurse into the current callback."
Section 13.2: "An event scheduled during a callback for `now` becomes eligible no
earlier than the next scheduler dispatch cycle."
D7: "Normalize to next tick, never recursive."

These are three different guarantees. "Next dispatch cycle" and "next tick" are
the same thing only if exactly one dispatch cycle runs per tick - which Section
13.3 explicitly denies, since a ready backlog left by budget exhaustion is
dispatched in a later main-loop iteration at the same tick.

The implementation normalizes to `current_tick + 1`, which satisfies D7 and
Section 10, and is strictly stronger than Section 13.2. But a future
optimization that made the ready list re-drainable within a tick would satisfy
13.2's letter and break D7.

**Required:** pick one normative statement - "normalized to `current_tick + 1`"
is the clearest - and make Sections 10 and 13.2 both defer to it.

### F14. The Section 11.1 state diagram omits a state the implementation has

`enum game_event_state` includes `GAME_EVENT_STATE_FAILED`, and
`game_event_result_failed()` is part of the public result API. Section 14.1
lists "Failed with a diagnostic classification" as a recurrence result.

The state chart in Section 11.1 has Created, Queued, Ready, Dispatching,
CancelPending, Completed, Cancelled, and Rejected. There is no Failed state and
no transition into it. A reviewer checking "cancellation cannot double-clean or
revive an event" - checklist item, currently marked complete - is checking an
incomplete diagram.

**Required:** add `Dispatching --> Failed` and `Failed --> [*]` to the chart, and
state in Section 15.2 whether a failed recurring event is cleaned up
immediately or retried.

### F15. Section 12 never confronts the three genuinely different owner lifetimes

Section 12.2 requires the entity registry to "Increment or replace generation on
lifecycle reuse" and Section 12.1 says `(runtime_id, generation)` identifies one
live incarnation. This is stated uniformly for all owner kinds.

The three kinds do not behave alike:

- Characters and objects are allocated and freed constantly. Generation is
  essential and the cost is per-entity.
- Rooms live in a static `world[]` array for the process lifetime and are
  effectively never freed. Generation is nearly meaningless, except during OLC
  world reload, which is exactly the case where it matters and is hardest.
- Regions, encounters, and vessels have subsystem-specific registries with
  their own lifetime rules, and `src/mud_event.c` already special-cases room
  and region VNUM copies (Section 4.3 notes this).

Section 12.3 gestures at this - "Static database identifiers and runtime
generations remain distinct concepts" - but assigns no per-kind rule. The Section
31 checklist item "Owner generation semantics cover PCs, NPCs, objects, rooms,
and runtime subsystem owners" is correctly left unchecked; this finding is why.

**Required:** a per-owner-kind table in Section 12: what increments generation,
what the runtime ID is, and what happens on OLC reload for rooms specifically.

### F16. `descriptor` owners are listed in the compatibility surface but excluded from the entity model

Section 4.3 lists descriptor among the MUD event owner lists, and `src/mud_event.c`
does maintain `d->events` (see `src/mud_event.c:535` and `:708`). Section 24.2
requires descriptor list queries to remain valid. Section 12.3 lists descriptor
among owners that "may use the same owner-handle shape."

Descriptors are the one owner kind whose lifetime is controlled by the reactor
being introduced in Phase 3, not by gameplay. A descriptor can be closed from
inside a reactor callback while a scheduled event still references it, and
copyover deliberately preserves descriptors across an `exec` while transient
events are being discarded.

**Required:** state the descriptor owner rule explicitly, including what happens
to descriptor-owned events across copyover, and reconcile it with Section 17's
transient/preserved classification.

### F17. Section 4.1's baseline omits the existing missed-pulse catch-up

Section 4.1 describes the main loop as `select()`-based at ten pulses per
second and notes `PASSES_PER_SEC`, `RL_SEC`, and pulse intervals. Confirmed
accurate - `src/structs.h:5504`.

What it omits is that `src/comm.c:1020` already tracks `missed_pulses` and the
loop already has catch-up behavior. Section 2's Principle 8 forbids "an unbounded
catch-up burst" and Section 14.2 builds an entire lateness policy taxonomy, but
neither is compared against what the loop does today. A reviewer cannot tell
whether Phase 3's compatibility timer preserves, changes, or silently drops the
existing catch-up semantics.

**Required:** document the current missed-pulse behavior in Section 4.1 and state
in Phase 3 whether the compatibility timer reproduces it.

### F18. Section 4.4's baseline is accurate but omits the payload hazard it implies

I verified Section 4.4 against source. `set_fighting()` attaches
`new_mud_event(eCOMBAT_ROUND, ch, "1")` at `src/combat/fight.c:1855`, and
`event_combat_round()` returns `2 RL_SEC` at `src/combat/fight.c:16557` with the
comment "6 second rounds, hack!". The three-phase counter description is correct.

The detail worth adding: that phase counter is a heap-allocated *string*,
mutated in place with `sprintf` into `pMudEvent->sVariables` at
`src/combat/fight.c:16552`. It survives only because the digits 1 through 3 are
all one byte. Section 9.3's rule that "New-style payloads MUST NOT use free-form
strings as their primary schema" is exactly right, and this is its best
motivating example. Section 21 would be stronger for citing it.

**Required:** optional, but citing this in Section 9.3 or 21.7 turns an abstract
rule into a concrete migration target.

### F19. `libevent` is selected (D15, Accepted) with no dependency contract

D15 accepts `libevent` as the reactor. Phase 3 says "Add the selected `libevent`
dependency to Autotools, CMake, setup, and deployment documentation."

Unspecified: minimum version, which sub-libraries (core only, or `event_pthreads`
for the Section 16 cross-thread ingress), license compatibility, static versus
system linkage, behavior when the dependency is absent at configure time, and
whether the `select()` fallback survives a build without `libevent` at all.

`configure.ac` and `CMakeLists.txt` currently contain no `libevent` reference,
so this is entirely greenfield. Given that the project's stated fallback strategy
is "select the existing `select()` driver at boot," the build must support
producing a binary with no `libevent` at all, or the rollback is theoretical.

**Required:** a dependency subsection in Phase 3: minimum version, components,
license, and the configure-time behavior when absent.

### F20. No CI story for the dual-driver period

Phases 3 through 11 maintain two I/O drivers and, per D10, two timed-event
backends. Section 24.9 lists the tooling gates but names no matrix.

If CI exercises only the default driver, the fallback path rots and the rollback
guarantee that Phases 3, 4, 7, 8, and 10 all depend on becomes unverified. That
is a documented risk with an undocumented mitigation.

**Required:** state in Section 24.9 that the production-linked suite runs under
both drivers, and both backends, for the duration of the migration.

---

## 4. Minor Findings and Nits

### F21. Section 17's shutdown sentence is ambiguous

"Shutdown cleanup drains no gameplay callbacks unless explicitly requested."
This can be read as "does not drain" or as "drains zero." Rewrite as "Shutdown
cleanup MUST NOT invoke gameplay callbacks unless a caller explicitly requests
a draining shutdown."

### F22. Section 19's facade claims MUD-event behavior that Phase 2 does not deliver

Section 19 says the legacy facade preserves "Existing owner-list query helpers"
and "Existing specialized cleanup until each owner type migrates." Phase 2's
deliverables cover only "legacy base-event creation." MUD events arrive in Phase
5. Section 19 should say which half of the facade lands in which phase.

### F23. Section 31's checklist is inconsistent with the document's own status

Five items are checked. "Current-state description matches the source" is
unchecked, yet Section 4 is presented as validated and dated. Either the
current-state review happened - in which case check it - or Section 4.8's
"Last source review: 2026-08-30" header claim overstates what was reviewed.

Note also that F1 through F4 in this review should reopen two items currently
marked complete: "Payload ownership and failed-admission behavior are explicit"
is fine, but "Time and lateness semantics are unambiguous for the standalone
core" is contradicted by F13, and "Wheel geometry and overflow behavior are
accepted for Phase 1" is contradicted by F2.

### F24. Section 8.1's geometry table is arithmetically correct

Verified: L0 64 ticks = 6.4s; L1 4,096 = 6.8 min; L2 262,144 = 7.28 h;
L3 16,777,216 = 19.4 days; L4 1,073,741,824 = 3.4 years. Section 8.2's placement
thresholds are consistent with it. `GAME_SCHEDULER_WHEEL_LEVELS` is 5 and
`GAME_SCHEDULER_WHEEL_SLOTS` is 64 in the header. No defect - recorded so the
next reviewer does not redo the arithmetic.

### F25. Test count claims verified

Section 23's Phase 1 record claims 22 tests in
`unittests/CuTest/test_game_scheduler.c`. Confirmed: 22 `Test*(CuTest *tc)`
functions. The file is registered in `Makefile.am:368` and `:426` and in
`CMakeLists.txt:1200`, and `src/game_scheduler.c` is in both build manifests
(`Makefile.am:115`, `CMakeLists.txt:523`), satisfying the repository rule that
both be updated. The claim that no call site outside its tests creates a
scheduler is also confirmed - `game_scheduler` appears in no other `.c` file.

### F26. Section 22's activity model defers its hardest question

"One character initially owns at most one primary intentional activity" (D21,
Provisional). The specification lists capability claims - movement, hands,
attention, vision, speech, and the action types - which is a resource model
capable of expressing concurrency. It then forbids concurrency by fiat.

The single-activity rule and the capability-claim model are two designs for the
same problem, and the document keeps both. That is defensible for Phase 10, but
Section 22 should say which one is authoritative when they disagree, or the
first implementer will have to guess.

### F27. Section 21.3's join rule needs a fourth case

"Parties have different encounters: merge the encounters." Section 21.5 covers
merge. Neither covers the case where a character is a participant in an
encounter that has already resolved to "no hostile sides remain" (Section 21.6)
but has not yet been torn down, because teardown is deferred to "after the
current dispatch" (Section 21.4, step 5). Joining a zombie encounter is a real
window.

### F28. The ongoing-projects index was stale against the branch

[`README_ongoing-projects.md`](README_ongoing-projects.md) described the
specification as "Draft architecture specification; implementation not started"
while this branch already carries a compiled, tested Phase 1 scheduler core and
the specification itself records that tranche in Section 23. The index row is
corrected in this change, and a row for this review is added.

---

## 5. What the Specification Gets Right

Recorded because an adversarial review that lists only defects misrepresents the
document.

- Every source claim I checked in Section 4 is accurate: the ten-pulse loop, the
  bucketed DG queue, `q_el == NULL` as the in-dispatch marker, the 10,000-event
  cap at `src/dgscript/dg_event.h:80`, the untyped `void *pStruct`, the
  `eCOMBAT_ROUND` two-second three-phase structure, and the pub/sub subsystem's
  character of being a database messaging system rather than a domain bus. This
  is unusually well grounded for a planning document.
- Section 2's central invariant - "Wall-clock frequency must never determine
  action economy" - is the correct thing to put at the top, and Sections 7.2,
  14.2, 21.7, and 22 all hold to it consistently.
- The separation of domain events from decision hooks (Sections 20.1, 20.3) is
  the right call and is stated with unusual precision. "A notification handler
  cannot report a failure that silently rolls the completed operation back" is
  the sentence that will prevent the most bugs in this project.
- Injecting both the tick source and the microsecond source
  (`game_scheduler_config`) rather than reading a clock internally is a better
  testability decision than the specification takes credit for.
- The tagged `game_event_result` replacing the overloaded integer return of
  `EVENTFUNC` is a genuine correctness improvement over the legacy contract, not
  a stylistic one.
- Section 23's insistence that "Backend migration and combat-rules redesign must
  not be combined in one release" (21.7) and the split of Phases 8 and 9 is
  exactly right and is the kind of discipline these projects usually lack.
- The Phase 1 record's own admission that the production delay trace is missing
  is honest self-reporting. F7 escalates it, but the document did not hide it.

---

## 6. Recommended Disposition

Do not accept version 1.0 as written.

Blocking before 1.0: F1, F2, F3, F4, F5.
Required before the Phase 1 gate closes: F1, F2, F6, F7, F11.
Required before Phase 3 begins: F3, F4, F9, F19, F20.
Everything else can be folded into a 0.5 revision.

The Phase 1 core is good work and should not be reverted. The gap is between
what the specification promises as a contract and what the accepted core
implements, and it is closable by amending the specification, moving the owner
index into Phase 1, or both.
