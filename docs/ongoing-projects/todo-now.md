# Production MUD Health Findings

Report time: 2026-08-16 06:40 UTC

Status: **Online, but application health is degraded.** The host has ample spare CPU,
memory, and disk capacity, and the game remained reachable throughout observation. The
important risks are main-loop latency, monotonically growing anonymous memory, and a
confirmed player-affect persistence defect.

## Scope and deployment identity

- Live observation window: approximately 2026-08-16 06:10-06:38 UTC.
- Additional evidence: production syslog from 2026-08-13 through 2026-08-16.
- Production process: PID 562989, `circle -C3 4100`, running as `luminari`.
- The PID began on 2026-08-13 and survived copyovers. The currently loaded image was
  installed by the 2026-08-16 06:07 UTC copyover.
- Version: `LuminariMUD 2.5061-beta (tbaMUD 3.64)`.
- Commit: `c7ab375fc91f7c634bfb744db57c62fd7e13be71`, clean build.
- ELF build ID: `5c2242b12830a22928dc76898dde05769187d4eb`.
- The deployed commit matches the current repository HEAD.

All checks were read-only except for creating this report. The production process was not
restarted, signaled, attached to a debugger, or reconfigured.

## Runtime summary

| Area | Observed state | Assessment |
| --- | --- | --- |
| Network | Port 4100 accepted a client probe; 9 established sessions had zero queued data | Healthy |
| Process | 2 threads, 38 open file descriptors out of a soft limit of 8192 | Healthy |
| CPU | Usually 49-59% of one core; work was almost entirely on the main thread | Degraded single-loop headroom |
| Host CPU | 8 logical CPUs; load average about 0.9-1.0 | Healthy host capacity |
| MUD RSS | About 1.45 GiB at the end of observation; approximately 98.6% was anonymous memory | Needs action |
| Host RAM | 15 GiB total, 7.3 GiB available, no swap | Adequate now, but no swap safety margin |
| Disk | 185 GiB available, 61% used | Healthy |
| MariaDB | Reachable; 1 cumulative slow query; no connection failure observed | Healthy availability |
| Kernel | No OOM kill, segfault, or storage error found in the recent journal | Healthy |
| Intermud/Terrain API | I3 heartbeats and local Terrain API requests completed successfully | Healthy |

The current image logged no fatal error, OOM, or crash during the observation window. It did
log one severe pulse, one critical pulse, and recurring catch-up exhaustion.

## Live load samples

### Ordinary activity

Repeated `pidstat` samples showed the MUD using approximately 49-59% of one core. On an
8-CPU host this is only about 6-7% of total host capacity, but the game loop is effectively
single-threaded, so total host idle capacity does not protect players from a blocked loop.

RSS increased from 1,466,516 KiB at about 06:10 to 1,518,424 KiB at 06:35. This is a
monotonic increase of 51,908 KiB, or about 2.0 MiB/minute over this short window.

### Zusuk combat window

The user deliberately started combat with Zusuk from 06:33:47 through 06:35:48 UTC.

| Metric | Combat result |
| --- | --- |
| Main-loop CPU | Mean 55.5% of one core; range 50.6-59.6% across 24 samples |
| RSS | Increased 1,828 KiB, about 0.92 MiB/minute |
| Connections | Remained at 9 |
| Catch-up | 13 budget exhaustions across 93 catch-up passes in two summaries |
| Severe/critical pulses | None during the combat capture |
| Fatal/OOM errors | None |
| MariaDB after combat began | 59.1 questions/sec and 34.1 SELECTs/sec in a 10-second sample |

Conclusion: combat did not cause a new CPU spike or acute main-loop failure. An artifact
owned by Zusuk reached level 3 at 06:33:26, and the next save at 06:38:07 again reported an
affect-capacity overflow. That timing is consistent with the artifact-affect defect described
below, but it does not by itself prove that this level-up caused the overflow.

## Findings

### 1. Player affects are being truncated, and artifact passives are persisting incorrectly

Severity: **High - confirmed player-state integrity defect**

Evidence:

- `MAX_AFFECT` is 32 in `src/structs.h:5706`.
- `save_char_checked()` copies only 32 affects, removes every live affect, logs the warning
  only when at least two more list nodes remain, then restores only the copied 32. See
  `src/players.c:2384-2428` and `src/players.c:4101-4105`.
- Production emitted `OUT OF STORE ROOM FOR AFFECTED TYPES` nine times after the current
  image loaded, most recently at 06:38:07.
- Zusuk's pfile is exactly full at 32 saved affect records. Thirty records use spell 1611,
  `SPELL_ARTIFACT_PASSIVE`; 29 of those are identical permanent +3 Will enhancement
  records. All 30 artifact-passive records have `specific=0`.
- The +3 Will row matches Vengeance level 3 in `src/obj/spec_artifacts.c:353-356`.
- `struct affected_type` has a runtime `source_id` (`src/structs.h:7245-7258`), and new
  artifact passives are applied with it (`src/obj/spec_artifacts.c:2320-2351`).
- Pfile affect version 1 writes 14 fields but omits `source_id`
  (`src/players.c:3917-3931`). The loader uses plain `affect_to_char()` and therefore reloads
  the record with no source owner (`src/players.c:4763-4844`).
- Artifact cleanup removes the current source-tagged affect, or an older record whose
  `specific` field matches the artifact. It cannot identify the persisted records where both
  `source_id` and `specific` are zero (`src/obj/spec_artifacts.c:2354-2378`).

Impact:

- Saves discard affects beyond the fixed 32-record buffer from both the pfile and the restored
  live character state.
- Repeated passive bonuses can inflate character statistics.
- Legitimate affects may be lost depending on linked-list order.
- The bad records survive copyover/reload and can be reapplied again.

Recommended fix:

1. Stop persisting equipment-derived artifact passives and rebuild them from equipped
   artifacts after load, or version the pfile format and persist/restore `source_id` safely.
2. Add a targeted migration that removes or deduplicates legacy source-less,
   `specific=0` artifact passive rows. Validate each affected character before modifying live
   or saved data.
3. Detect and reject duplicate derived passives before application.
4. Make overflow diagnostics include the character name and actual count. Do not treat merely
   raising `MAX_AFFECT` as the root fix.
5. Add production-linked tests for save/load, copyover, artifact level-up, equip/unequip, and
   duplicate cleanup.

### 2. Synchronous player saves freeze the single game loop for multiple seconds

Severity: **High - confirmed player-visible latency**

At 06:09:11, one heartbeat took 4,154,687 usec (4.15 seconds). The built-in profile recorded:

| Section | Time |
| --- | ---: |
| `Crash_save_all` | 2,671,171 usec |
| `msdp_update` | 126,465 usec |
| `event_process` | 68,614 usec |
| `House_save_all` | 60,559 usec |
| `pulse_luminari` | 19,364 usec |

The same synchronous crash-save pattern appears repeatedly in historical logs:

- 2026-08-13: 1.52 seconds and 2.17 seconds.
- 2026-08-14: 2.70 seconds.
- 2026-08-15: 2.65 seconds.
- 2026-08-16: 2.67 seconds.

A staff `save` command also consumed 334.9 ms inside a 552.3 ms severe pulse at 06:08:48.

The source confirms that the heartbeat invokes `Crash_save_all()` and `House_save_all()`
synchronously when the autosave interval expires (`src/comm.c:1851-1863`).
`Crash_save_all()` serially saves every dirty connected player (`src/obj/objsave.c:1868-1882`).

Since the 06:07 copyover, 30 catch-up summaries recorded 687 budget-exhaustion events across
4,020 catch-up passes (17.1%). Long synchronous work is directly contributing to delayed
pulses and recovery backlog.

Recommended fix:

1. Instrument save time per player and separate object-save time from character-file time.
2. Make autosave incremental and budgeted across pulses.
3. Move file preparation or safe I/O work outside the main loop where practical, while
   preserving atomic and durable player saves.
4. Add latency alerts for pulses over 100 ms, 500 ms, and 1 second.

### 3. Wilderness map generation creates a synchronous per-tile SQL storm

Severity: **High when a player is in the wilderness; confirmed query and movement latency**

The every-second heartbeat calls `msdp_update()` (`src/comm.c:1618-1623`). For every playing
descriptor, it rebuilds affects, actions, group, inventory, room data, the automap, the graphic
map, and the wilderness graphic map (`src/comm.c:5269-5331`). This work is reached without
first checking whether the client reports the specific map variable. Room updates also invoke
the map builders, so a move can generate a map there and again in the next every-second update.

The wilderness map radius is 8, producing a 17x17 or 289-tile map. `get_map()` calls both
`get_enclosing_regions()` and `get_enclosing_paths()` for every tile
(`src/wilderness/wilderness.c:497-533`). Each helper performs a synchronous spatial SELECT on
the global MariaDB connection (`src/mysql.c:2415-2462` and `src/mysql.c:2951-2992`). A single
map build can therefore issue up to 578 SELECTs.

Observed correlation:

- While Zusuk was moving around wilderness coordinates near `(-85, 102)`, production logged
  movement/look command times of 182-322 ms.
- Global MariaDB `Com_select` was 535.4/sec during a wilderness sample, close to one map's
  theoretical maximum of 578 queries/sec.
- Global question rates were commonly 527-621/sec and briefly reached 1,112/sec.
- `msdp_update` took 126-131 ms in the severe/critical pulse profiles.
- After Zusuk moved into combat outside that wilderness path, a fresh sample fell to 34.1
  SELECTs/sec while MUD CPU stayed near 55% of one core.

The database rates are global, not tagged to the MUD connection, so attribution is an
evidence-backed inference. The source path, query-count match, movement timings, and sharp
drop outside the wilderness make that inference strong. The unchanged CPU during combat also
shows that this SQL path is not the sole cause of steady process CPU.

Recommended fix:

1. Build a map only when the client supports MSDP/GMCP and has subscribed to that variable.
2. Rebuild only when coordinates, room, visibility, or map data changes. Remove duplicate
   generation on movement and the next one-second update.
3. Replace 578 per-tile queries with preloaded in-memory spatial data or one/two batched
   bounding-box queries per map.
4. Cache map results by center, radius, plane, and world-data revision.
5. Profile each map builder separately and add a regression test asserting zero map SQL on an
   unchanged tick and a small bounded query count after movement.

### 4. Anonymous RSS is growing monotonically

Severity: **High if sustained; cause not yet isolated**

The process gained 51,908 KiB RSS over roughly 25.5 minutes. Growth was monotonic in the
samples, although its rate varied: about 2.0 MiB/minute across the full window and 0.92
MiB/minute during the two-minute combat window. Approximately 98.6% of RSS was anonymous,
and `pmap` showed growth in the large private anonymous mapping.

This is enough to confirm retention or heap growth, but not enough to label a specific
allocation as a leak. Short-window rates must not be extrapolated as a forecast. The build
does not enable `MEMORY_DEBUG`, and deployed map routines resolve to libc `calloc`/`free`, so
the debug allocator that intentionally retains freed blocks is not the explanation.

Monitoring need:

The project needs longer-running memory monitoring because this short observation cannot
distinguish an unbounded leak from allocator retention, cache growth, fragmentation, or a
legitimate one-time high-water mark. The monitoring must provide enough history to determine:

- Whether RSS and anonymous memory eventually stabilize or continue growing.
- How the growth rate changes during representative player activity, wilderness use, combat,
  saves, idle periods, and copyovers.
- Whether memory is released after the activity that caused it has ended.
- Whether growth correlates with game population or internal resource counts.
- Whether the retained memory creates an operational capacity risk over the server's normal
  lifetime.

The evidence must be preserved as a time series so later investigation can focus on the code
paths active when memory grew. This report intentionally does not select a monitoring design,
sampling interval, profiler, deployment mechanism, or retention period. Those choices should
be made separately after agreeing on the operational requirements and acceptable production
overhead.

### 5. Typed special-procedure contracts disagree with world flags and gateways

Severity: **Medium - log flood and invalid dispatch work**

From the 06:07 copyover through 06:37, the live image logged:

- 554 `Vampire Cloak` unsupported object-auto-pulse errors.
- 53 `RoL Lavatubes Mobile` unsupported command errors.

The Vampire Cloak supports command and identify events only
(`src/spec/spec_registry.c:84-86`), but its object prototype has `ITEM_AUTOPROC`, so
`proc_update()` dispatches an auto-pulse (`src/comm.c:1561-1576`). The worn-then-carried
fallback in `src/spec/spec_dispatch.c:367-396` explains the usual pair of identical errors
every approximately six seconds.

The Lavatubes mobile is registered with the activity-only `janitor_events` contract
(`src/spec/spec_registry.c:1673-1683`), while the generic mobile command gateway still tries
to dispatch player commands to it. This becomes visible whenever a player issues commands in
a room containing those mobiles.

Recommended fix:

- Remove `ITEM_AUTOPROC` from the Vampire Cloak prototype unless real pulse behavior is
  intended; otherwise add an actual supported pulse handler.
- Make typed command gateways preflight event support and return zero silently for an event
  that the definition does not declare. Preserve diagnostics for genuinely invalid contexts.
- Add registry/world validation at boot so incompatible prototype flags fail validation once
  instead of flooding runtime logs.

### 6. An MSDP macro defect falsely reports memory allocation failure

Severity: **Low - confirmed protocol defect and misleading diagnostic**

The current image logged seven pairs of:

- `ExecuteMSDPPair: Invalid MSDP string limit`
- `ExecuteMSDPPair: Failed to allocate MSDP value buffer`

This was not an OOM. `STRING_WRITE_ONCE(x, y)` ignores `x` and `y` and expands to limits
`-1, -1` (`src/net/protocol.c:156`). `CLIENT_ID` and `CLIENT_VERSION` use that macro. The
setter correctly rejects the invalid maximum, deliberately leaves the buffer null, and then
emits the generic allocation message (`src/net/protocol.c:3514-3526`).

Recommended fix: make `STRING_WRITE_ONCE(x, y)` expand to `x, y`, and distinguish invalid
configuration from actual allocation failure. Add table validation during protocol startup.

## Healthy signals

- The game remained reachable and retained 9 established connections throughout monitoring.
- No connection backlog, descriptor exhaustion, thread explosion, OOM, crash, or kernel fault
  was found.
- Host CPU, RAM, and disk have substantial current headroom.
- MariaDB, I3 heartbeat/presence, and the Terrain API were available.
- Zusuk's added combat load did not trigger a severe/critical pulse or a material CPU change.

## Recommended action order

1. Protect player data: fix artifact-passive persistence/deduplication and perform a controlled
   cleanup of affected pfiles and live characters.
2. Remove multi-second main-loop saves by instrumenting and incrementally scheduling them.
3. Gate, cache, and batch MSDP map generation, especially wilderness spatial lookups.
4. Establish long-running memory monitoring and use its evidence to scope the subsequent
   retention investigation.
5. Correct the Vampire Cloak and Lavatubes typed-event mismatches.
6. Correct the `STRING_WRITE_ONCE` macro and misleading protocol error.

## Post-fix verification criteria

- Zusuk and other players survive save, logout/login, and copyover with no duplicate artifact
  passives and no affect truncation warning.
- A wilderness movement builds at most a small bounded number of database queries, and an
  unchanged tick builds no map or runs no map SQL.
- No normal command exceeds 100 ms because of map generation.
- Autosave no longer creates pulses over 500 ms; ideally it stays below the 100 ms pulse
  budget.
- Catch-up budget exhaustion trends toward zero during ordinary operation.
- RSS plateaus during a representative long-running observation period, or any continued
  growth has an identified and acceptable bound.
- No recurring typed-special or false MSDP allocation errors appear in syslog.

## Monitoring limitations

- Kernel sampling with `perf` was unavailable because the host has
  `kernel.perf_event_paranoid=4`; the report uses the MUD's built-in pulse profiler, process
  counters, logs, and source tracing instead.
- MariaDB status counters are global to the database server. Query attribution is based on
  before/after rates combined with the traced synchronous query path.
- No production mutation or intrusive heap instrumentation was performed, so the exact owner
  of retained anonymous memory remains open.
