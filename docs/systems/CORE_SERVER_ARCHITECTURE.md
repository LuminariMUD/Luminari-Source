# LuminariMUD Core Server Architecture

## Overview

LuminariMUD uses a single-threaded, event-driven server architecture based on
the classic CircleMUD/tbaMUD design. A private reactor waits for descriptor,
signal, scheduler, or queued-action deadline readiness; gameplay remains on the
main thread. Named scheduler services own normal cadence work. A 100 ms
compatibility heartbeat remains available only for explicit rollback and the
legacy timed-event backend.

## Main Server Components

### 1. Main Entry Point (`main()` function)

The server startup sequence follows this pattern:

```c
int main(int argc, char **argv)
{
    // 1. Parse command line arguments
    // 2. Load configuration
    // 3. Setup logging
    // 4. Change to data directory
    // 5. Initialize game or run syntax check
    // 6. Cleanup and shutdown
}
```

**Key Command Line Options:**
- `-f <file>` - Specify a configuration file using a safe library-relative path
- `-o <file>` - Specify log file
- `-d <dir>` - Set data directory
- `-C<socket>` - Copyover recovery mode
- `-s` - Syntax check mode only
- `<port>` - Port number to listen on

### 2. Game Initialization (`init_game()`)

The initialization sequence is critical for proper server startup:

```c
static void init_game(ush_int local_port)
{
    // 1. Create kill script protection
    touch(KILLSCRIPT_FILE);

    // 2. Initialize random number generator
    circle_srandom(time(0));

    // 3. Determine player limits
    max_players = get_max_players();

    // 4. Setup network socket (unless copyover)
    if (!fCopyOver)
        mother_desc = init_socket(local_port);

    // 5. Initialize timed events and seal typed domain-event contracts
    event_init();
    domain_event_runtime_init();

    // 6. Setup character lookup hash table
    init_lookup_table();

    // 7. Load world data
    boot_db();

    // 8. Admit named runtime services
    runtime_services_init();

    // 9. Setup signal handlers
    signal_setup();

    // 10. Handle copyover recovery if needed
    if (fCopyOver)
        copyover_recover();

    // 11. Enter main game loop
    game_loop(mother_desc);

    // 12. Shutdown sequence
    // - Save all player data
    // - Close all sockets
    // - Save world state
    // - Clean shutdown or reboot
}
```

### 3. Main Game Loop (`game_loop()`)

The heart of the server is the main game loop, which handles all server operations:

```c
void game_loop(socket_t local_mother_desc)
{
    fd_set input_set, output_set, exc_set, null_set;
    struct timeval timeout, last_time, opt_time;

    while (!circle_shutdown) {
        // 1. Handle no-connection sleep state
        // 2. Setup file descriptor sets
        // 3. Calculate timing for next iteration
        // 4. Arm the nearest scheduler, queued-wait, or rollback deadline
        // 5. Wait for readiness (libevent, with select rollback)
        // 6. Accept new connections
        // 7. Handle exceptions and disconnections
        // 8. Process input from all descriptors
        // 9. Execute commands and game logic
        // 10. Process output to all descriptors
        // 11. Run compatibility heartbeat only when rollback requires it
        // 12. Dispatch a bounded scheduler batch
        // 13. Drain deferred extraction at the explicit safe point
        // 14. Performance monitoring
    }
}
```

**Timing System:**
- **Runtime resolution:** monotonic 100 ms ticks, without a mandatory 100 ms wake
- **Compatibility heartbeat:** `OPT_USEC` only for runtime-service or legacy-backend rollback
- **Sleep mechanism:** reactor readiness plus the nearest scheduler or queued-wait deadline
- **Timed work:** generation-aware owner events on the scheduler or legacy rollback queue
- **Typed facts:** synchronous, bounded domain events from committed state changes

The target architecture does not schedule every player command. Commands remain
direct interpreter work. Timings, regeneration, automatic actions, combat,
activities, AI, and active-world processing migrate to owner-scheduled work;
typed domain facts wake only affected owners. Phases 7 through 11 remove broad
discovery scans where ownership is available and isolate legitimate global
maintenance as named work.

## Network Architecture

### Socket Management

The server uses BSD sockets for network communication:

**Mother Socket (`mother_desc`):**
- Listens for new incoming connections
- Bound to specified port (default varies by configuration)
- Non-blocking mode for connection acceptance

**Descriptor Management:**
- Each client connection gets a `descriptor_data` structure
- Linked list of all active descriptors (`descriptor_list`)
- File descriptor sets for `select()` system call

### Connection Handling

```c
// New connection acceptance
if (FD_ISSET(local_mother_desc, &input_set))
    new_descriptor(local_mother_desc);

// Input processing
for (d = descriptor_list; d; d = next_d) {
    if (FD_ISSET(d->descriptor, &input_set)) {
        if (process_input(d) < 0)
            close_socket(d);
    }
}

// Output processing  
for (d = descriptor_list; d; d = next_d) {
    if (*(d->output) && FD_ISSET(d->descriptor, &output_set)) {
        if (process_output(d) < 0)
            close_socket(d);
    }
}
```

### Input/Output System

**Input Processing:**
- Raw socket data read into buffers
- Line-based command parsing
- Alias expansion and command queuing
- State-based processing (login, playing, editing, etc.)

**Output Processing:**
- Buffered output system with queues
- Color code processing
- Protocol handling (telnet, MSDP, etc.)
- Flow control and connection management

### Operational Health Endpoint

The existing Terrain API listener binds only to loopback and shares the main
select-driven game loop. It accepts HTTP health requests in addition to its
newline-delimited JSON terrain protocol:

- `GET /health` and `GET /health/ready` return HTTP 200 only when the main
  MariaDB connection responds; otherwise they return HTTP 503.
- `GET /health/live` returns HTTP 200 once the initialized game loop is
  servicing the listener and does not query MariaDB.
- Health responses are JSON, disable caching, and close the connection.

The listener defaults to port 8182. `TERRAIN_API_PORT` can select another
unprivileged port, while `scripts/operations/healthcheck.sh` consumes the
matching `LUMINARI_HEALTH_URL` for systemd and operator checks.

### Future Native WebSocket Considerations

Luminari-Source does not currently document or expose a native WebSocket
listener. Adding one would affect the core networking model rather than only
the protocol documentation. A future design would need to define whether the
listener carries raw terminal text, a new source-owned application protocol, or
compatibility with the Luminari Web `/ws` JSON contract.

Before any public listener is enabled, source maintainers should define and test
these boundaries:

- Descriptor creation, polling, output writes, close handling, and cleanup for
  WebSocket descriptors alongside Telnet descriptors.
- Frame-to-input behavior, including text and binary frame handling, line
  buffering, parser isolation, malformed payloads, and command throttling.
- Copyover behavior for WebSocket descriptors, including whether they restore
  protocol state or intentionally disconnect with a clear rollback path.
- Browser-facing origin/auth expectations, WSS/TLS termination, health checks,
  connection quotas, privacy-safe logging, and sanitized observability.
- Rollback to the integrated Luminari Web proxy until source-native behavior has
  equivalent security, operations, parser, and client-contract coverage.

## Runtime Service Scheduling

Normal cadence work is admitted as named service-owned events at its established
interval. Only services required by the selected subsystem modes are scheduled:

```c
service.dg_random             // 13 seconds
service.one_second            // protocol, travel, crafting, and related work
service.mobile_procedures     // established mobile-procedure cadence
service.zone                  // zone reset cadence
service.thirty_second         // periodic character/object maintenance
service.minute_persistence    // starts bounded save cycles
service.persistence_batch     // dynamic one-operation persistence step
service.mud_hour              // weather, quests, diplomacy, point work
service.mud_day               // daily world work
```

Additional definitions preserve moving rooms, idle-password checks, usage and
time saves, hunt creation, and subsystem rollback callbacks. A default live
boot currently admits 14 of 24 definitions because migrated owner systems do
not need their rollback service. Startup admission is all-or-nothing; failure
restores `LUMINARI_RUNTIME_SERVICES=legacy` behavior for that boot.

Named services preserve existing routines and ordering within each cadence.
Some are legitimate global maintenance or still call bounded connected/active
owner traversals; the service name does not imply every inner routine is
owner-local.

## Signal Handling

The server handles various UNIX signals for administrative control:

```c
static void signal_setup(void)
{
    signal(SIGUSR1, reread_wizlists);   // Reload wizard lists
    signal(SIGUSR2, checkpointing);     // Force save checkpoint  
    signal(SIGINT, hupsig);             // Graceful shutdown
    signal(SIGTERM, hupsig);            // Graceful shutdown
    signal(SIGPIPE, SIG_IGN);           // Ignore broken pipes
    signal(SIGALRM, SIG_IGN);           // Ignore alarm signals
    signal(SIGCHLD, reap);              // Clean up child processes
}
```

**Signal Functions:**
- `reread_wizlists()` - Reload administrator lists
- `checkpointing()` - Force save all player data
- `hupsig()` - Initiate graceful shutdown
- `reap()` - Clean up zombie child processes

## World Loading (`boot_db()` and `boot_world()`)

The world loading process is complex and follows a specific order:

```c
void boot_world(void)
{
    // 1. Database connection
    connect_to_mysql();

    // 2. Core world data
    index_boot(DB_BOOT_ZON);    // Zones
    index_boot(DB_BOOT_TRG);    // Triggers  
    index_boot(DB_BOOT_WLD);    // Rooms

    // 3. Extended world features
    load_regions();             // Geographic regions
    load_paths();               // Travel paths
    renum_world();              // Renumber rooms

    // 4. Entities
    index_boot(DB_BOOT_MOB);    // NPCs/Monsters
    index_boot(DB_BOOT_OBJ);    // Objects/Items

    // 5. Game systems
    load_class_list();          // Character classes
    assign_feats();             // Feat system
    load_deities();             // Religion system

    // 6. Finalization
    renum_zone_table();         // Final zone numbering
    boot_social_messages();     // Social commands
    load_help();                // Help system
}
```

### Special-Procedure Boot Control Plane

Special-procedure setup is a compatibility control plane around the existing single callback slot
on mobile, object, and room prototypes. It has three separate state layers:

| Layer | Owner | Authority |
|-------|-------|-----------|
| Immutable definition | `src/spec/spec_registry.c` | Stable identity, aliases, owners, events, prerequisites, visibility, and allowed sources. |
| Authored binding | Prototype-owned `struct spec_binding` | Exact world or explicit OLC request used by persistence and content diagnostics. |
| Effective history | Prototype-owned `struct spec_effective_binding` | Ordered observation of boot callback writes, collisions, wrappers, and final slot. |

The actual function pointer on `room_data` or mobile/object `index_data` remains runtime dispatch
authority. Neither metadata record invokes callbacks, changes return handling, or creates a handler
chain.

The relevant `boot_db()` sequence is:

```text
spec_registry_boot_validate()
  -> spec_assign_table_boot_validate()
  -> boot_world()
       -> named world bindings
       -> moving-room parser hooks
  -> if specials are enabled:
       assign_mobiles()
       assign_the_shopkeepers()
       assign_objects()
       assign_rooms()
       assign_the_quests()
  -> report_effective_spec_bindings()
```

Registry and declarative-table validation are programmer-error boundaries and run before any world
file is parsed. Every table row must resolve to a definition compatible with its owner and permitted
for legacy assignment. Unknown or owner/source-incompatible world names are content errors: the
owned authored record and source location remain available, but no callback is installed.

The Phase 02 table contains the two Luminari assignments that have both a traced symbolic VNUM and a
registered definition. Numeric, computed, and campaign-compatibility assignments remain direct
calls, but both forms use the same owner-specific assignment helpers and effective-history recorder.
The table is a validated compatibility source, not a new precedence level. Phase 07 placed those
helpers in `src/spec/spec_assign.c` and split the compiled inventory into mobile, object, and room
owners. `src/spec/spec_assign.h` remains the only public assignment boot interface.

Phase 03 source extraction does not change this control flow. General object callbacks now live in
`src/spec/spec_objects.c`; legacy route, ferry, and Greyhawk ship callbacks live in
`src/vessels/vessels_legacy.c`. Legacy moving-room `M` loading, runtime-list ownership, zone-pulse
scheduling, relocation helpers, and callback live together in
`src/vessels/vessels_moving_rooms.c`. Player shops, commerce and item services, crafting molds,
vampire-cloak customization, quest reward replacement, and the Neverwinter control puzzle live with
their respective feature owners. Ability calculations and skill list/training behavior live under
`src/character/`, and spell sorting and list display live under `src/magic/`. General legacy mobile
and room callbacks plus reusable combat/companion archetypes live under `src/spec/`; class guild,
clan-hall, wizard spellbook research, and pet-shop services live with their character, clan, magic,
and object owners. The King's Castle assignment, mobile callbacks, private helpers, and runtime
state live together in `src/spec/spec_zone_kings_castle.c`; Abyss exit randomization and Crimson
Flame encounter behavior live in their corresponding `src/spec/spec_zone_*` owners. The Prisoner
raid state, death transition, treasury loader, and callbacks live in `src/spec/spec_zone_prisoner.c`;
the Celestial Leviathan no-op callback and dormant helpers live in their own adjacent owner. Fire
Giant invasion construction and the transforming instrument live together in
`src/spec/spec_zone_fire_giant.c`. Jot invasion state, position tables, relative-VNUM conversion,
group and equipment loading, mobile encounters, and zone-specific objects live together in
`src/spec/spec_zone_jot.c`. Mad Drow cube-slider flags, row tables, exit mutation helpers,
cube-wide messages, and callback live together in `src/spec/spec_zone_mad_drow.c`. TTF AOE
encounters, follower summoning, patrol route, and per-mobile path scheduling live in
`src/spec/spec_zone_ttf.c`; that final package retired `src/zone_procs.c`. The public names, callback
ABI, world grammar, pulse position, assignment and registry references, initialization order,
calculations, and display behavior remain unchanged. The Shadow Dragon combat-pulse callback lives
in `src/spec/spec_zone_shadow_dragon.c`, and the Banshee one-shot wail lives in
`src/spec/spec_zone_banshee.c`. Quicksand room effects live in
`src/spec/spec_zone_quicksand.c`. The Tower of Kenjin mobile encounter and rotating rooms live
together in `src/spec/spec_zone_kenjin_tower.c`, while the Hive of Passion death transition lives
in `src/spec/spec_zone_hive_of_passion.c`. Fey-Branche combat coordination lives in
`src/spec/spec_zone_feybranche.c`, and Abyssal Vortex exit rotation lives in
`src/spec/spec_zone_abyssal_vortex.c`. House Agrach-Dyrr combat coordination lives in
`src/spec/spec_zone_agrach_dyrr.c`, and House Shobalar coordination lives in
`src/spec/spec_zone_shobalar.c`. Earth Plane reinforcement behavior lives in
`src/spec/spec_zone_earth_plane.c`, and Air Plane combat and reinforcement behavior lives in
`src/spec/spec_zone_air_plane.c`. The Zusuk Fzoul callback lives in
`src/spec/spec_zone_zusuk.c`, and the Orc Ruins Shar callbacks live in
`src/spec/spec_zone_orc_ruins.c`. Illithid Enclave access control lives in
`src/spec/spec_zone_illithid_enclave.c`. Kobold Caverns, Bandit Castle, and Secomber access guards
live in `src/spec/spec_zone_kobold_caverns.c`, `src/spec/spec_zone_bandit_castle.c`, and
`src/spec/spec_zone_secomber.c`. Longsaddle, Flaming Tower, Mere of Dead Men, and Battlemaze
behavior lives in `src/spec/spec_zone_longsaddle.c`, `src/spec/spec_zone_flaming_tower.c`,
`src/spec/spec_zone_mere_of_dead_men.c`, and `src/spec/spec_zone_battlemaze.c`. Fire Plane, Water
Plane, and Snake Pit publish distinct owner APIs while deliberately sharing
`src/spec/spec_zone_alarm_group.c`, which keeps `zone_yell()` private beside all three consumers.
Menzoberranzan movement and Narbondel state live in `src/spec/spec_zone_menzoberranzan.c`. All
twenty-five post-TTF packages have dedicated owner APIs, and the final four retire the last compiled
callbacks from `src/spec_procs.c`.
The shared `is_wearing()` equipment predicate now lives in `src/handler.c` beside equip and unequip
operations; its legacy same-VNUM semantics are unchanged.
After a residual dormant-code audit, `src/spec_procs.c` was removed from both build manifests;
Phase 07 then removed the top-level assignment source and declaration umbrella. All cross-module
consumers include the narrow header owned by the assignment, registry, feature, vessel, or zone
implementation they use. The completed ownership evidence is in
[Special Procedure Phase 03 Validation](../testing/SPECIAL_PROCEDURE_PHASE_03_VALIDATION.md) and
[Special Procedure Phase 07 Validation](../testing/SPECIAL_PROCEDURE_PHASE_07_VALIDATION.md).

Phase 04 adds narrow mechanics below the gateway and above existing engine primitives. Typed
payload validation in `src/spec/spec_context.c` rejects malformed owner/event shapes before legacy
translation and provides shared exact-worn-object and live-combat checks. `spec_phrase.c` preserves
the characterized byte-exact phrases of its opt-in consumers. `spec_cooldown.c` names the legacy
object `spec_timer[]` clock as MUD-hour, instance-owned, bounded, and nonpersistent.
`spec_combat.c` retains the raw `damage()` result while classifying no effect, applied damage, and
possible target invalidation. `spec_effects.c` stores stable negative source ownership in
`affected_type.source_id` and a separate spell-scoped stacking group in `specific`; artifact
passives and temporary powers are its first consumers. Artifact progression and persistence remain
owned by `src/obj/spec_artifacts.c`.

The weapon-hit caller passes its actual victim into the gateway instead of reconstructing it from
ambient combat state. Legacy callback translation remains synchronous and ABI-compatible. The
complete mechanic, lifetime, and consumer matrix is in
[Special Procedure Phase 04 Validation](../testing/SPECIAL_PROCEDURE_PHASE_04_VALIDATION.md).

Phase 05 adds mixed typed and legacy dispatch behind the unchanged prototype callback slot. Each
definition now validates as either one complete legacy handler or a unique callback-slot adapter
paired with a typed handler. Every gateway calls `spec_dispatch()`, which reverse-resolves typed
adapters and otherwise performs exact legacy translation. Bank and Vampire Cloak are the first typed
consumers; they use explicit item-identify event identity, and Vampire Cloak validates the exact
invoking worn object. Their persisted names, callback pointers, assignments, and builder rows remain
unchanged. The registry reports 2 typed and 26 legacy definitions; the source tree still contains
194 legacy behavior implementations. See
[Special Procedure Phase 05 Validation](../testing/SPECIAL_PROCEDURE_PHASE_05_VALIDATION.md).

Phase 06 audits optional composition and lifecycle expansion. It keeps the existing runtime-only
`questmaster -> shop_keeper -> original callback` nesting and does not add a persisted general chain:
each prototype and authored binding still has one callback identity. It also adds no zone/world
special-procedure events. DG Scripts cover localized lifecycle content, while artifact and vessel
boot/reset work remains a direct call into its owning subsystem. The complete decision and reopen
criteria are in
[Special Procedure Phase 06 Validation](../testing/SPECIAL_PROCEDURE_PHASE_06_VALIDATION.md).

The guarded assignment order preserves quest-over-shop-over-original composition. Shop and quest
wrappers record the actual callback saved in `SHOP_FUNC` or `QST_FUNC`; they are not flattened into a
general chain. Under `-s`, world names and parser hooks still load while the assignment block is
skipped. Effective reporting remains outside that block and labels the mode, so it describes the
path that ran without becoming a new global dispatch gate.

Each contribution emits a bounded `SPEC_BIND` line. `SPEC_BIND_FINAL` records the latest authored
request, contribution and collision counts, and final source/handler. `SPEC_BIND_SUMMARY` brackets
the report and provides aggregate counts. Text inputs are owned, single-line validated, and escaped
before formatting.

Immortal staff can inspect one prototype's recorded post-boot history with
`specbind <mob|obj|room> <vnum>`. The command is read-only and does not recompute the boot snapshot.

Authored and effective records follow prototype lifetimes. Database shutdown, prototype deletion,
OLC scratch cleanup, room insertion, and room copying use explicit deep-copy/free operations. World
writers consult authored identity first and use reverse handler lookup only for a legacy prototype
that has no authored record.

Moving-room `M` data is a parser-owned callback with a different `me` payload from room procedures.
It cannot share the room callback slot with a named `Z` binding. Boot rejects both field orders,
REdit blocks selection and defensive internal save, and the room writer preflights the complete zone
before opening output or mutating mover state.

Phases 00-06 do not introduce multiple-handler dispatch. Two handlers are typed behind stable
adapters; remaining conversions must preserve the characterized scheduling, traversal, activation,
return, and precedence rules. Optional composition or shared lifecycle events reopen only for an
approved consumer with complete ordering, lifetime, OLC, and persistence coverage.

## Performance Monitoring

The server's single-threaded game loop targets ten pulses per second. `src/perfmon.c` records
outer-loop utilization, named profiling sections, event-queue activity, pending-character
extraction, and missed-pulse recovery. The data is available through automatic high-water log
reports and the implementor-only `perfmon` command.

Named sections use the following pattern:

```c
/* The macro initializes the static section pointer on first use. */
PERF_PROF_ENTER(pr_main_loop_, "Main Loop");
/* Profiled work. */
PERF_PROF_EXIT(pr_main_loop_);
```

### Performance Monitoring Architecture

The monitor has related bounded data sets:

- Outer-loop utilization rolls pulse samples into bounded second, minute, and hour circular
  buffers. `PERF_log_pulse()` expresses elapsed work as a percentage of the 100 ms pulse target.
- Named sections have per-pulse and cumulative call, time, and maximum counters. Sampling can be
  enabled explicitly for rolling percentile data.
- Game-loop telemetry has per-pulse and cumulative counters for event queue depth, callbacks,
  callbacks created while processing, pending extractions, and catch-up requests.
- A 128-row slow-pulse flight recorder correlates schedule classes, top-level sections,
  main-thread SQL, callbacks, entity inventory, and requested/replayed/dropped heartbeats.
- SQL telemetry separates main and worker threads and reports bounded latency samples, errors,
  reconnects, normalized table families, and explicit subsystem categories without SQL values.
- Entity telemetry records mobile/object construction and extraction centrally, then ranks positive
  net growth by VNUM, zone, and explicit creation reason. Zone reset timing and population deltas
  share this reset window.
- A one-day minute ring records memory, entities, queries, and pulse-tail counters for short,
  medium, and long slope analysis.

`PERF_prof_reset()` clears only the current pulse. `PERF_reset()` starts a new cumulative
measurement window and also resets utilization, missed-pulse, vessel-message, event, extraction,
and catch-up counters. Startup performs this reset, along with the database-query counter reset,
after world boot and copyover recovery finish. The initial operational memory baseline therefore
includes the normally resident world instead of reporting world-load allocation as live growth.

Before an accepted copyover starts descriptor handoff persistence, PERFMON captures the current
memory sample and atomically replaces `log/perfmon-pre-copyover.txt`. The snapshot contains a
readable summary, persistence state, max/p99 rankings, and complete profiling, SQL, slow-pulse,
combat, memory, and entity CSV sections. It uses a same-directory temporary file plus flush,
`fsync()`, and `rename()`, so a failed capture leaves the preceding complete snapshot intact.
Snapshot failure is logged but does not prevent copyover. The fixed destination intentionally
retains only the newest run.

Event callback profiling uses a fixed 512-identity registry. Names are registered when events are
created, so callback execution does not perform a string lookup. Human and CSV reports expose the
registered count, registry capacity, top-16 report limit, and unregistered overflow calls. Callback
rows are ranked by cumulative execution time and include call count, total, average, p50, p95,
p99, and maximum microseconds.

The active-world manager gives every autonomous in-world NPC one owner-scoped deadline distributed
across the six-second mobile interval. Player proximity does not define activity: patrols,
wandering, scripts, special procedures, and NPC wars continue off-screen. Movement and combat facts
re-evaluate only their directly affected owners and rooms; extraction cancels the owner. See
[`ACTIVE_WORLD.md`](ACTIVE_WORLD.md).

DG Script wait events rely on the attachment lifetime contract: extracting an attached script
cancels `GET_TRIG_WAIT`, and room relocation updates wait owners. A resumed wait therefore enters
the script driver directly instead of scanning all characters, objects, or rooms to rediscover its
owner.

Automatic high-water reports are rate limited by severity. Catch-up diagnostics are aggregated into
at most one log line per five seconds while every pass remains represented in PERFMON counters.

Minute persistence no longer executes an all-player burst. The scheduler snapshots stable player
IDs and advances character, dirty-pet, last-online, dirty-artifact, crash-object, and dirty-house
work one operation per pulse. It rotates task classes for fairness, skips disconnected players,
retries failures, targets 20 ms, and reports operations above the 50 ms diagnostic threshold. An
individual synchronous save cannot be preempted, so the threshold is an observable acceptance gate
rather than a claim that a storage operation is forcibly interrupted.

Account state has independent dirty generations for core data, character membership, race unlocks,
and class unlocks. Successful writes advance only the saved generations; ordinary character
serialization no longer rewrites account unlock tables. Pet saves use a stable state fingerprint so
unchanged owners avoid the delete-and-reinsert transaction.

Eligible-owner registries remove four measured cadence scans: autonomous NPCs, `ITEM_AUTOPROC`
objects, DG random-trigger owners by mobile/object/room type, and affected characters plus rooms.
Each eligible affected character owns one round-boundary duration event; all room affects in one
room share one event. Connected-player MSDP remains connection work and is refreshed by the existing
one-second descriptor path and immediate affect mutations. Registry updates occur at the same attach,
detach, flag-change, extraction, and OLC replacement boundaries that change eligibility. Staff entity
reports perform debug full-list validation; normal scheduled paths do not. See
[`PERIODIC_OWNER_EVENTS.md`](PERIODIC_OWNER_EVENTS.md) and
[`AFFECTED_OWNER_EVENTS.md`](AFFECTED_OWNER_EVENTS.md).

The mud-hour point service adds no per-entity event. One aligned deadline makes
the global/player/object phases due; an intrusive registry contains every PC,
and another contains only timer, timer-trigger, imbued-missile, decay, and
corpse objects. Direct lifecycle and mutation hooks maintain both registries.
This preserves weather/time-trigger ordering and the old owner routines while
removing normal `character_list` and `object_list` discovery scans. Set
`LUMINARI_POINT_UPDATE_EVENTS=legacy` at boot for exclusive whole-list
rollback. See [`MUD_EVENTS.md`](MUD_EVENTS.md).

Combat callback timing separates action queues, attack generation, assist fan-out, specials, and
backlash. A bounded slow-combat ring stores safe numeric context for callbacks above 100 ms. Per
round attack and special-procedure limits prevent recursive proc chains from monopolizing the game
loop and report every truncation.

### Monotonic Runtime Ticks and Rollback

`game_loop()` derives the global runtime tick from `CLOCK_MONOTONIC` elapsed
time, independently of callback execution. Scheduler deadlines therefore remain
eligible after a quiet reactor sleep without replaying empty 100 ms turns.
`WAIT_STATE` consumes the elapsed tick delta, and queued input or a pending
action adds its exact wait expiry to the reactor timeout.

The legacy timed-event backend still needs one compatibility tick to advance
its queue. `LUMINARI_RUNTIME_SERVICES=legacy` restores the whole heartbeat and
its bounded catch-up behavior. PERFMON records runtime advances separately from
legacy requested, replayed, and dropped heartbeats.

### Staff Command

The `perfmon` command requires `LVL_IMPL`:

- `perfmon all`: decision-oriented summary, save scheduler, slow pulses, SQL, ranked sections,
  combat, entities, sweeps, memory history, and database query count.
- `perfmon summ`: utilization summary only.
- `perfmon prof`: cumulative named sections, game-loop telemetry, and database query count.
- `perfmon mem`: current memory state and one-day minute history.
- `perfmon slow [count] [csv]`: recent over-budget pulse correlations.
- `perfmon sql [csv]`: SQL ownership and latency telemetry.
- `perfmon entities [csv]`: entity lifecycle, zone reset, sweep, and registry telemetry.
- `perfmon combat [count] [csv]`: slow combat records and chain-limit counters.
- `perfmon saves`: incremental scheduler state and high-maximum save sections.
- `perfmon top <total|max|p99> [limit]`: ranked profiling sections.
- `perfmon csv`: all machine-readable profiling, SQL, pulse, combat, memory, and entity rows.
- `perfmon sect <section>`: one named section.
- `perfmon reset`: start a new measurement window.

The event queue line reads `depth=initial->latest`; `max_before` and `max_after` show interval
peaks. `dropped_missed` is work discarded on that pass and is not carried forward. A high callback
total with a modest average points to callback volume; a high maximum with
few calls points to one expensive invocation. Timed casting checks extraction state directly;
pending-extraction cleanup clears casting target references in the same batched world pass used for
combat, guarding, hunting, and last-attacker references. Casting callback cost therefore does not
scale with the total number of loaded characters.

The memory dashboard and CSV inventory include affected characters, individual affect nodes, NPC
followers, charmed NPCs, and entity deltas since `perfmon reset`. Compare those deltas with heap and
anonymous RSS growth before classifying a rise as allocator leakage. A representative production
run is still required to prove that a population and memory cohort reaches its intended plateau.

### API Functions

```c
/* Outer-loop utilization. */
void PERF_log_pulse(double val);

/* Report generation. */
size_t PERF_repr(char *out_buf, size_t n);
size_t PERF_slow_repr(char *out_buf, size_t n, size_t count, int csv);
size_t PERF_sql_repr(char *out_buf, size_t n, int csv);
size_t PERF_entities_repr(char *out_buf, size_t n, int csv);
size_t PERF_combat_repr(char *out_buf, size_t n, size_t count, int csv);
size_t PERF_prof_repr_pulse(char *out_buf, size_t n);
size_t PERF_prof_repr_total(char *out_buf, size_t n);
size_t PERF_prof_repr_top(char *out_buf, size_t n, const char *metric, size_t limit);
size_t PERF_prof_repr_sect(char *out_buf, size_t n, const char *id);
size_t PERF_prof_repr_csv(char *out_buf, size_t n);

/* Section management, normally used through PERF_PROF_ENTER/EXIT. */
void PERF_prof_sect_init(struct PERF_prof_sect **ptr, const char *id);
void PERF_prof_sect_enter(struct PERF_prof_sect *ptr);
void PERF_prof_sect_exit(struct PERF_prof_sect *ptr);
void PERF_prof_reset(void);

/* Bounded game-loop telemetry. */
int PERF_register_event_callback(const char *identity);
void PERF_note_event_callback(int profile_index, uint64_t elapsed_usec);
void PERF_note_event_process(uint64_t depth_before, uint64_t depth_after,
                             uint64_t callbacks_processed, uint64_t events_created);
void PERF_note_pending_extractions(uint64_t pending_before, uint64_t processed,
                                   uint64_t pending_after);
void PERF_note_catchup_pass(uint64_t requested_missed, uint64_t replayed_missed,
                            uint64_t remaining_backlog, int budget_exhausted);
```

Event callback timing is always on and costs two monotonic clock reads per invocation. Registry,
sample, interval, and report sizes are fixed, so monitoring memory cannot grow with uptime. Keep
game-loop counters in `perfmon.c`: they share the same pulse/cumulative reset and report lifecycle.
Splitting them into another module would add ownership boundaries without isolating an independent
subsystem.

## Memory Management

**Buffer Pools:**
- Pre-allocated buffer pools for common operations
- Reduces malloc/free overhead
- Configurable buffer sizes

**Descriptor Management:**
- Linked list of active connections
- Automatic cleanup on disconnect
- Memory leak prevention

**World Data:**
- Static allocation for world structures
- Reference counting for shared data
- Cleanup functions for shutdown

## Error Handling and Logging

**Logging System:**
- Configurable log levels
- Timestamped entries
- Separate error streams
- Rotation and archival support

**Error Recovery:**
- Graceful handling of network errors
- Automatic cleanup of corrupted connections
- Core dump generation for debugging
- Restart mechanisms for critical failures

## Copyover System

LuminariMUD supports "copyover" - restarting the server without disconnecting players:

1. **Preparation:** Save all critical state
2. **Socket Transfer:** Pass socket descriptors to new process
3. **Restart:** Execute new server binary
4. **Recovery:** Restore player connections and state
5. **Continuation:** Resume normal operation

This allows for seamless updates and maintenance without player disruption.

The built-in nightly copyover check uses UTC and normally executes at 08:00 UTC, with warnings at
07:30, 07:45, 07:55, 07:58, and 07:59 UTC. Happy hour or an active staff event suppresses that
minute's automatic action. An accepted automatic or staff-initiated copyover first refreshes the
single PERFMON snapshot described above; recovery then starts a new monitoring window.

## Configuration System

The server uses a flexible configuration system:

- **Config Files:** Text-based configuration files
- **Runtime Changes:** Some settings can be changed while running
- **Default Values:** Sensible defaults for all settings
- **Validation:** Input validation and error checking

## Thread Safety

**Important Note:** LuminariMUD is single-threaded by design. All operations must be thread-safe or avoided entirely. This simplifies development but requires careful attention to:

- Non-blocking I/O operations
- Efficient algorithms to prevent lag
- Proper resource management
- Event-driven programming patterns

The single-threaded design ensures data consistency and simplifies debugging while maintaining excellent performance for typical MUD workloads.
