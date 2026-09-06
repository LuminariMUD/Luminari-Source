# Assigned issue repair batch

Base: `origin/master` at `55d19e0bd`, branch `fix/open-issue-repairs`.
Scope: open issues assigned to `jamclaug` on 2026-09-06, #103 through #112.
The wider unassigned queue is excluded. No production rollout or archival deletion is authorized.

## Implementation checkpoint

- First implementation commit: `7a09f5d8a` (committed world operations and Nature).
- User reaffirmed that migrations must use the existing native event system.
  Crafting now runs through the existing primary activity manager and
  event_runtime; no second scheduler is being introduced. Supply refresh uses
  its existing wall-clock timestamps lazily rather than a descriptor scan.
  New tests run without descriptor-list membership and verify offline pause/
  reconstruction and committed-move cancellation. Resize completion uses the
  originally admitted object, not another inventory object sharing its prototype.
  The full suite passes 1,145 tests. Buff, transport, mover and staff migration
  remain in #105; no full-issue completion is claimed.
- #111 workload and thresholds are declared in
  [the performance gate](../testing/EVENT_CORE_ASSIGNED_BATCH_PERFORMANCE_GATE.md)
  before final-revision measurements.
- #112 local inventory and retention disposition are recorded in the existing
  [release gate](../deployment/EVENT_DRIVEN_CORE_RELEASE_GATE.md#8-assigned-batch-retention-review-2026-09-06).
  Production retirement remains gated; this batch does not delete archival data.

- #104: relocation causes and caller-owned operation capture are implemented. Walking
  waits for entry/greet/wall decisions; temporary `at` commands returning to their
  original room are silent. Nested relocation, extraction and stale identities are
  covered by contract tests, plus a real entry-trigger veto test. Caller audit and
  final validation are continuing.
- #103: typed holder facts and compound transfer capture are implemented for room,
  inventory, equipment, container and character bag paths. Give/get/put/drop/wear/
  remove have operation boundaries. Command, script, magic, shop, reset and restore
  contexts retain actor/cause. Give re-resolves all participants after each veto.
  Nested transfer, bag no-op, scoped extraction and real quest delivery tests pass.
  Existing discovery and delivery consumers now run after commit; consumption stops
  after one item. Detached movement origins retain stable room handles.
- #110: Nature remains canonical; Survival is a legacy alias for slot 29. Camp,
  feat/perk descriptions and eight flat/database help records were reconciled. The
  additive help migration was applied and compared against the local development
  database; previous affected records were backed up privately under /tmp.
- The expanded `make test` suite passed all 1,141 tests on 2026-09-06.
  `make install` installed the development binary; no server was restarted.
  The changed-line formatting pass was followed by another passing full build/test.
  No full-batch completion or performance acceptance is claimed by this checkpoint.

See [committed-operation contracts](../systems/COMMITTED_WORLD_OPERATIONS.md).

## #103: Events: publish one atomic object-transfer fact with holder and actor context

Source: https://github.com/LuminariMUD/Luminari-Source/issues/103

Status: under review.

`ObjectMoved` in `src/domain_event_runtime.c` describes room placement/removal through `handler.c`. Inventory, equipment and containers lack a complete transfer fact. A consumer cannot reliably distinguish one delivery from the removal and insertion halves.

- [ ] Define typed source/destination holders (room, character inventory, equipment slot, container), actor and cause where known.
- [ ] Trace player, NPC, script, spell, shop and extraction paths; emit once after the full transfer commits. No-op/failed transfers emit nothing.
- [ ] Keep pre-operation vetoes separate; re-resolve generation-safe identities after callbacks.
- [ ] Test nested transfers, extraction, unavailable holders and exactly-once quest delivery credit before migrating a reward consumer.

Source review: `c7c7d44a7f47e5fc155859eaf359391b827f85ea`. The temporary working notes are being removed; these revision-pinned links retain their evidence and detailed examples. Historical measurements are not fresh runtime results.

- [EVENT_GAMEPLAY_OPPORTUNITIES_AND_AUDIT_2026_09_05.md](https://github.com/LuminariMUD/Luminari-Source/blob/c7c7d44a7f47e5fc155859eaf359391b827f85ea/docs/ongoing-projects/EVENT_GAMEPLAY_OPPORTUNITIES_AND_AUDIT_2026_09_05.md)


## #104: Events: preserve relocation cause and direction in committed movement facts

Source: https://github.com/LuminariMUD/Luminari-Source/issues/104

Status: under review.

`handler.c` publishes `CharacterMoved` with direction -1 from low-level placement. Walking, teleportation, forced movement, spawning and scripted relocation are indistinguishable; placement can precede higher-level script decisions.

- [ ] Introduce relocation context through authoritative callers and document the committed operation boundary.
- [ ] Preserve typed pre-move decisions for blocking traps/opportunity attacks; a post-move fact cannot undo movement.
- [ ] Cover walking, spells, forced/scripted movement, startup, failed/no-op moves, nested relocation and extraction with production-linked tests.

Source review: `c7c7d44a7f47e5fc155859eaf359391b827f85ea`. The temporary working notes are being removed; these revision-pinned links retain their evidence and detailed examples. Historical measurements are not fresh runtime results.

- [EVENT_GAMEPLAY_OPPORTUNITIES_AND_AUDIT_2026_09_05.md](https://github.com/LuminariMUD/Luminari-Source/blob/c7c7d44a7f47e5fc155859eaf359391b827f85ea/docs/ongoing-projects/EVENT_GAMEPLAY_OPPORTUNITIES_AND_AUDIT_2026_09_05.md)


## #105: Events: migrate remaining active player, mover and staff countdown loops

Source: https://github.com/LuminariMUD/Luminari-Source/issues/105

Status: under review.

The native clock is complete, but feature discovery/countdowns remain. See `docs/systems/EVENT_MECHANISM_INVENTORY.md` for their current owners and cadence.

- [x] Replace descriptor polling for crafting (`craft_update`) and self-buff sequences with owned activities; preserve cancellation, material costs, timing and offline policy.
- [x] Replace transport `travel_tickdown` with passenger/job deadlines; transit need not occupy a passenger's primary activity.
- [x] Give supply refresh an explicit online/offline policy and next deadline or justified lazy timestamp.
- [x] Replace `movingRoomList` countdown discovery with owned mover deadlines.
- [x] Give active staff events named agendas for delay, expiry, portals and population replenishment.
- [ ] Keep inactive owners unscheduled; test logout, copyover, owner extraction, restart reconstruction and admission failure; expose semantic reasons in `eventdebug`.

Retain useful shared mud-hour cadence, lazy evaluation and external I/O ingress. The removed legacy scheduler/heartbeat needs no further retirement.

Source review: `c7c7d44a7f47e5fc155859eaf359391b827f85ea`. The temporary working notes are being removed; these revision-pinned links retain their evidence and detailed examples. Historical measurements are not fresh runtime results.

- [EVENT_GAMEPLAY_OPPORTUNITIES_AND_AUDIT_2026_09_05.md](https://github.com/LuminariMUD/Luminari-Source/blob/c7c7d44a7f47e5fc155859eaf359391b827f85ea/docs/ongoing-projects/EVENT_GAMEPLAY_OPPORTUNITIES_AND_AUDIT_2026_09_05.md)
- [EVENT_DRIVEN_CORE_REFACTOR_PHASE7_SCAN_INVENTORY.md](https://github.com/LuminariMUD/Luminari-Source/blob/c7c7d44a7f47e5fc155859eaf359391b827f85ea/docs/ongoing-projects/EVENT_DRIVEN_CORE_REFACTOR_PHASE7_SCAN_INVENTORY.md)


## #106: Gameplay: design counterspell decisions and ally-protection ready triggers

Source: https://github.com/LuminariMUD/Luminari-Source/issues/106

Status: under review.

Timed casting activities and typed readied normal attacks on entry, door opening and timed casting are implemented. This tracks the remaining tactical design, not reimplementation of those tranches.

- [ ] Define a counterspell decision window before resolution, including identification, eligibility, resource costs and competing interruptions.
- [ ] Define designated-ally attack triggers using an attack-attempt/outcome contract, not a damage fact alone.
- [ ] Explicitly decide whether/how instant casts are interruptible; current CastingStarted applies only to committed timed casts.
- [ ] Preserve standard-action reservation, one strike, expiry and exact cast/target identities; do not grant another reaction allowance.
- [ ] Test simultaneous deadlines, visibility/range changes, cancellation/extraction, failed admission, encounter transitions and once-only execution.

Broader spell/turn/healing/condition/skill facts should be added only with a concrete consumer and authoritative ordering.

Source review: `c7c7d44a7f47e5fc155859eaf359391b827f85ea`. The temporary working notes are being removed; these revision-pinned links retain their evidence and detailed examples. Historical measurements are not fresh runtime results.

- [EVENT_GAMEPLAY_OPPORTUNITIES_AND_AUDIT_2026_09_05.md](https://github.com/LuminariMUD/Luminari-Source/blob/c7c7d44a7f47e5fc155859eaf359391b827f85ea/docs/ongoing-projects/EVENT_GAMEPLAY_OPPORTUNITIES_AND_AUDIT_2026_09_05.md)
- [EVENT_GAMEPLAY_TRANCHE_2_CASTING.md](https://github.com/LuminariMUD/Luminari-Source/blob/c7c7d44a7f47e5fc155859eaf359391b827f85ea/docs/ongoing-projects/EVENT_GAMEPLAY_TRANCHE_2_CASTING.md)
- [EVENT_GAMEPLAY_TRANCHE_3_READIED_ATTACKS.md](https://github.com/LuminariMUD/Luminari-Source/blob/c7c7d44a7f47e5fc155859eaf359391b827f85ea/docs/ongoing-projects/EVENT_GAMEPLAY_TRANCHE_3_READIED_ATTACKS.md)


## #107: Gameplay: define turn-relative effects and movement-hazard exposure

Source: https://github.com/LuminariMUD/Luminari-Source/issues/107

Status: under review.

Affects have native owners but retain cadence-based durations. Existing walls, clouds, tentacles, traps and room damage need explicit tactical exposure rules.

- [ ] Define caster-relative, subject-relative and world/time duration policies and semantic start/end-of-turn phases.
- [ ] Pilot one-round defenses, bleeding and recurring saves; remove the old decrement for each migrated effect.
- [ ] Preserve remaining duration when encounters merge/end or actors leave.
- [ ] Define crossing, entry and continued-turn exposure per hazard, with effect-source identity and accounting that prevents duplicate damage.
- [ ] Depend on committed relocation context; keep blocking traps and opportunity decisions before movement.
- [ ] Test movement/re-entry, forced moves, vanished sources, encounter transitions and exactly-once exposure. This is a gameplay/rules design proposal, not an assertion that every current tick is defective.

Source review: `c7c7d44a7f47e5fc155859eaf359391b827f85ea`. The temporary working notes are being removed; these revision-pinned links retain their evidence and detailed examples. Historical measurements are not fresh runtime results.

- [EVENT_GAMEPLAY_OPPORTUNITIES_AND_AUDIT_2026_09_05.md](https://github.com/LuminariMUD/Luminari-Source/blob/c7c7d44a7f47e5fc155859eaf359391b827f85ea/docs/ongoing-projects/EVENT_GAMEPLAY_OPPORTUNITIES_AND_AUDIT_2026_09_05.md)


## #108: Gameplay: add bounded NPC perception and outcome-based quest consumers

Source: https://github.com/LuminariMUD/Luminari-Source/issues/108

Status: under review.

WorldPhenomenon propagation currently sends descriptions to players; NPC agendas do not consume a typed perception result. Quest gateways remain direct movement/item/death checks.

- [ ] Add source identity and phenomenon kind, then a bounded perception result using senses, stealth, obstacles, distance and faction knowledge; never parse descriptive prose as behavior.
- [ ] Pilot alarm investigation, ally warning, cover and timed loss of interest without waking the whole world.
- [ ] Classify DG/special/quest gateways as pre-operation decisions or post-operation notifications before bridging them.
- [ ] Pilot active-objective subscriptions for delivery, nonlethal resolution, rescue/negotiation, skill outcomes and witnessed actions.
- [ ] Preserve killer/pet/group credit, persistence and exactly-once rewards; migrate one authoritative award path at a time. Witness consequences must depend on perception.
- [ ] Add only the typed facts required by the chosen consumers, after the object-transfer and relocation contracts are complete.

Source review: `c7c7d44a7f47e5fc155859eaf359391b827f85ea`. The temporary working notes are being removed; these revision-pinned links retain their evidence and detailed examples. Historical measurements are not fresh runtime results.

- [EVENT_GAMEPLAY_OPPORTUNITIES_AND_AUDIT_2026_09_05.md](https://github.com/LuminariMUD/Luminari-Source/blob/c7c7d44a7f47e5fc155859eaf359391b827f85ea/docs/ongoing-projects/EVENT_GAMEPLAY_OPPORTUNITIES_AND_AUDIT_2026_09_05.md)


## #109: Gameplay: extend owned activities to skill work, guarded rest and expeditions

Source: https://github.com/LuminariMUD/Luminari-Source/issues/109

Status: under review.

Establish Camp and timed casting demonstrate the primary activity manager. Further exploration mechanics remain proposals.

- [ ] After active crafting/buff migrations, evaluate lockpicking, disarming, searching, climbing, first aid and rituals as interruptible work.
- [ ] Specify hands/movement/attention requirements, progress retention and material-consumption boundaries per activity.
- [ ] Design guarded rest with watches, interruptions, weather/exposure, supplies and staged recovery.
- [ ] Give expeditions/world events named departure, arrival, encounter, warning and expiry deadlines; persist authoritative state and reconstruct handles.
- [ ] Test interruption, resource accounting, target loss, logout and restart.

Changing rolling daily-use recovery to rest-based recovery is a separate balance decision; do not silently change it during migration.

Source review: `c7c7d44a7f47e5fc155859eaf359391b827f85ea`. The temporary working notes are being removed; these revision-pinned links retain their evidence and detailed examples. Historical measurements are not fresh runtime results.

- [EVENT_GAMEPLAY_OPPORTUNITIES_AND_AUDIT_2026_09_05.md](https://github.com/LuminariMUD/Luminari-Source/blob/c7c7d44a7f47e5fc155859eaf359391b827f85ea/docs/ongoing-projects/EVENT_GAMEPLAY_OPPORTUNITIES_AND_AUDIT_2026_09_05.md)


## #110: Rules/help: reconcile the Nature display name with Survival-based camp mechanics

Source: https://github.com/LuminariMUD/Luminari-Source/issues/110

Status: implemented and validated locally; additive help migration ready for deployment.

The audit found `ABILITY_SURVIVAL` and the displayed Nature ability sharing persisted slot 29. Establish Camp uses Survival terminology while the earlier rename presents Nature.

- [x] Choose the intended player-facing name and scope using the existing skill rules and help.
- [x] Reconcile camp messages, ability displays, documentation and help (database plus `lib/text/help/help.hlp`).
- [x] Preserve the persisted slot and existing character investment; do not split or renumber it implicitly.
- [x] Verify lookup aliases and existing saves after the naming decision.

Source review: `c7c7d44a7f47e5fc155859eaf359391b827f85ea`. The temporary working notes are being removed; these revision-pinned links retain their evidence and detailed examples. Historical measurements are not fresh runtime results.

- [EVENT_DRIVEN_CORE_REFACTOR_SPEC.md](https://github.com/LuminariMUD/Luminari-Source/blob/c7c7d44a7f47e5fc155859eaf359391b827f85ea/docs/ongoing-projects/EVENT_DRIVEN_CORE_REFACTOR_SPEC.md)


## #111: Performance: close the qualified event-core latency and memory acceptance gate

Source: https://github.com/LuminariMUD/Luminari-Source/issues/111

Status: under review.

Functional event-core acceptance passed, but the retained reports explicitly qualify performance. Late-callback counts do not measure lateness magnitude or end-to-end responsiveness, and short RSS samples do not establish a leak trend.

The September 5 burn-in recorded one 151.808 ms loop out of 9,654 samples (0.01%); DG waits accounted for 133.183 ms and extraction for 17.339 ms in that sample. This is a historical investigation lead, not proof of a recurring current defect.

- [ ] Agree representative idle and burst workloads and acceptance thresholds before measuring.
- [ ] Collect per-type deadline lateness, command/network tail latency and sustained RSS trends with build/configuration/workload provenance.
- [ ] Use `eventdebug ready [reset]` bounded 1,024-sample pulse p50/p95/p99/max appropriately: it excludes intentional delay and cannot prove sub-pulse or network latency.
- [ ] Investigate repeatable DG/extraction tail stalls if reproduced; retain negative findings as measurements.
- [ ] Update the existing acceptance report with evidence sufficient for an unqualified verdict or explicit remaining limitations.

Related: #93 concerns the production build profile; this issue concerns runtime acceptance measurements.

Source review: `c7c7d44a7f47e5fc155859eaf359391b827f85ea`. The temporary working notes are being removed; these revision-pinned links retain their evidence and detailed examples. Historical measurements are not fresh runtime results.

- [EVENT_DRIVEN_CORE_FINAL_RUNTIME_VALIDATION.md](https://github.com/LuminariMUD/Luminari-Source/blob/c7c7d44a7f47e5fc155859eaf359391b827f85ea/docs/ongoing-projects/EVENT_DRIVEN_CORE_FINAL_RUNTIME_VALIDATION.md)
- [EVENT_DRIVEN_CORE_REFACTOR_OBSERVABILITY_VALIDATION.md](https://github.com/LuminariMUD/Luminari-Source/blob/c7c7d44a7f47e5fc155859eaf359391b827f85ea/docs/ongoing-projects/EVENT_DRIVEN_CORE_REFACTOR_OBSERVABILITY_VALIDATION.md)
- [burnin-journal.md](https://github.com/LuminariMUD/Luminari-Source/blob/c7c7d44a7f47e5fc155859eaf359391b827f85ea/docs/ongoing-projects/burnin-journal.md)


## #112: Operations: decide retention and retirement of archival PubSub SQL data

Source: https://github.com/LuminariMUD/Luminari-Source/issues/112

Status: under review.

The old PubSub runtime, commands and build wiring are already retired. Archival SQL remains intentionally; its presence is not a second gameplay scheduler.

Follow `docs/deployment/EVENT_DRIVEN_CORE_RELEASE_GATE.md`: inventory actual tables/rows and readers, decide retention, export and verify a restorable backup, rehearse restoration and any proposed drop in isolation, and obtain the explicit operator decision before deleting data. Record the disposition in the existing release documentation. This issue does not authorize a production drop or removal of old event-save readers.

Source review: `c7c7d44a7f47e5fc155859eaf359391b827f85ea`. The temporary working notes are being removed; these revision-pinned links retain their evidence and detailed examples. Historical measurements are not fresh runtime results.

- [EVENT_DRIVEN_CORE_REFACTOR_PHASE11N_GATE_HANDOFF.md](https://github.com/LuminariMUD/Luminari-Source/blob/c7c7d44a7f47e5fc155859eaf359391b827f85ea/docs/ongoing-projects/EVENT_DRIVEN_CORE_REFACTOR_PHASE11N_GATE_HANDOFF.md)
- [EVENT_DRIVEN_CORE_ROLLBACK_QUARANTINE.md](https://github.com/LuminariMUD/Luminari-Source/blob/c7c7d44a7f47e5fc155859eaf359391b827f85ea/docs/ongoing-projects/EVENT_DRIVEN_CORE_ROLLBACK_QUARANTINE.md)

## Next implementation checkpoint: transport and remaining #105 owners

Crafting/supply commit: `c961729cf`; operational review commit: `b49fcbea6`.
The latest completed full test has 1,145 passing cases and was followed by
`make install`. No production process was restarted.

Transport trace:

- `enter_transport` in `vessels/transport.c` moves eligible followers and leader
  into a free room in the existing transit room pool. `travel_tickdown` scans
  descriptors once per second to deliver passengers.
- Destination currently means a room-array index. No travel fields are saved
  by players.c. A restored player in transit loses the destination and uses the
  existing emergency return path. `act.informative.c` reads travel_timer for ETA.
- Carriage/sailing commands charge the fare before transport admission. An
  unavailable transit room or invalid destination can therefore consume a fare
  without starting a journey. Admission must precede that charge.
- A transport job should be a native owned event, separate from primary
  activity. Reuse event_runtime ownership, cancellation, registration and
  diagnostics. Do not put the global loop behind another timer or introduce
  another timing service.
- Use a stable destination vnum in authoritative trip state, runtime handles
  for the current passenger/transit/destination incarnations, and a versioned
  save record for destination, remaining seconds, travel type and locale.
  Offline time should pause remaining time, matching the current descriptor
  behavior. Login/reconnect/copyover reconstruct the native deadline.
- Use a native named `transport.arrival` type. Admit the group before movement
  and payment; cancel any provisional jobs on failed admission. Capture the
  eligible follower identities before callbacks. NPC pets follow existing pet
  movement rather than receiving player-special trip state.
- Clear trip ownership before arrival notifications; re-resolve the passenger
  after pets/entry/greet callbacks. A committed move out of the transit room
  cancels the pending arrival. ETA is calculated from the native deadline.
- Relevant integration points: domain_event_runtime init/shutdown before type
  sealing; free_char; close_socket before save; CON_PLAYING transitions in
  interpreter.c and copyover_recover; players.c load/save; transport ETA display.

Other #105 research retained for the next work:

- Self-buff sequences call timed casting, which already owns primary activity.
  The sequence must coordinate native wakeups without claiming a second primary
  activity, granting actions twice, or retaining a raw target pointer. The old
  slot-skip loop indexes before checking its bound; start sets buffing flags
  before validating target location. Correct both during migration.
- Supply refresh is now lazy. The previous inventory's online-only claim was
  wrong: should_refresh_supply_slots uses time(NULL) and the last refresh time.
- Moving rooms currently scan movingRoomList every ten seconds and decrement
  remainingZonePulses. Give each loaded mover a native owned deadline at
  resetZonePulse * ten seconds, reconstruct after boot, and audit OLC teardown.
- Staff events have one active global event. Separate named expiry/delay from
  jackalope population maintenance and prisoner portal/atmosphere agendas,
  preserving their current mud-hour cadence.

#106-109 remain outstanding beyond the existing quest consumer migration.
#111 has declared thresholds but no final-revision latency/RSS measurements.
#112 retains archival SQL; production inventory and retirement approvals are
not inferred from the empty local database.

## Native transport implementation checkpoint

Transport now uses the existing event_runtime with a named, character-owned
`transport.arrival` deadline. The descriptor-wide travel_tickdown scan is
removed. This is background ownership: it can coexist with a primary activity
without creating another action allowance or scheduler.

- Group jobs are admitted before charging a fare or moving passengers. Failed
  admission cancels only jobs admitted by that attempt. Passenger and room
  identities are resolved across callbacks; a replacement room cannot inherit
  an old trip.
- Runtime state holds generation-checked passenger, transit and destination
  handles. The versioned `Trv1` player record stores destination vnum, remaining
  seconds, transport type and locale. Room-array indexes are not persisted.
- Disconnect snapshots remaining time and cancels the event. Login, reconnect
  and copyover reconstruct it. Offline time pauses travel. Old transit saves
  without trip state retain the existing emergency-return behavior.
- Committed relocation outside the transit room cancels the journey. Character
  cleanup and runtime shutdown release jobs through native cancellation. ETA
  comes from the native deadline. Missing destinations report a staff-assistance
  message instead of moving into a recycled room.
- Production-linked tests cover deadline arrival, coexistence with primary
  activity, offline pause/resume, scripted relocation, destination generation
  replacement, real player-file load/save, and group fare/admission rollback.

The installer integration test also now waits for its background helper to
exec the immutable release before inspecting /proc. Its cleanup is bounded;
this fixes an observed launch race and indefinite wait during validation.

Remaining #105 work is buff coordination, moving rooms and staff events.
The other outstanding issue work and performance/release gates above remain
open; transport does not complete the assigned batch.

Buff continuation trace for the next implementation:

- Reuse DOMAIN_EVENT_ACTIVITY_TRANSITIONED. activity_manager.c publishes the
  terminal transition after detaching ownership and running completion/ended
  callbacks. A continuation should match the exact casting activity ID and
  enqueue its next native wakeup, never cast reentrantly from publication.
- Timed casting already validates target, concentration, interruption and
  resources through start_casting_activity and do_gen_cast/do_manifest. Buff
  coordination must keep those entry points authoritative.
- spell_parser.c currently mutates GET_BUFF_TIMER for augmentation and Fabricate
  Focus. Remove this duplicate estimate when actual casting completion drives
  continuation; preserve deliberate inter-spell pacing and rapid-buff policy.
- GET_BUFF_TARGET is currently a raw character pointer; extraction scans player
  membership in char_from_buff_targets. Use a stable selected target identity
  and stop on lost/moved targets rather than silently casting on self. Audit
  the buff target command, including its unreachable empty-target branch.
- Validate the slot bound before GET_BUFF indexing. Validate the target before
  setting IS_BUFFING. Disconnect/restart policy must be explicit; saved buff
  lists are independent of an active sequence.

Transport checkpoint validation (2026-09-06): final `make -j10 test` passed,
including 1,151 gameplay cases and the installer/parser/monitor checks, with
no compiler warnings. `make install` succeeded afterward. No game server was
restarted. Branch remains `fix/open-issue-repairs`, based on master.

## Buff sequence migration checkpoint

The descriptor-wide self_buffing scan and GET_BUFF_TIMER estimates are removed.
A character-owned `buff.sequence.next-cast` event admits each next spell through
do_gen_cast/do_manifest. Timed casts retain their existing primary activity;
the sequence waits for that exact activity's terminal transition and queues
the next continuation after completion. It never casts from event publication
or claims an additional primary activity/action allowance. Instant casts retain
normal sequence pacing, including rapid-buff and Battle Blessing adjustments.

Active target and owner handles are generation-checked; native wakeups and
participant subscriptions also match the sequence incarnation. Movement of
either participant, missing target, interruption, disconnect and shutdown end
the sequence. Buff lists stay saved; active sequences are not restored after
restart/copyover. A selected target changed during a sequence applies to the
next sequence. The legacy selected-target pointer remains protected by existing
extraction cleanup; active sequences do not retain it.

Sparse lists check bounds before indexing. Admission validates participants and
busy state before setting active flags. Failed admission does not consume the
rapid-buff affects. The buff target command's empty-argument reset now works.
The flat help entry and local development database now describe this policy;
`sql/components/help_buff_sequence_entry.sql` carries the deployable update.

Native sparse-list/target movement/busy/offline cases and actual prepared-cleric
casting/interruption tests pass. The sequence has no pending next-cast event
while casting; the exact activity transition drives continuation. Interrupted
casting preserves the later prepared spell and does not resolve the first.
Target disambiguation also passes a real casting test with two identically named
NPCs. Lookup uses NPC keywords and verifies the ordinal through the same
character lookup as casting. Final `make -j10 test` passed on 2026-09-06:
1,158 gameplay tests and all invoked integration checks, with no compiler
warnings. `make install` succeeded afterward. No game process was restarted.

Moving-room follow-up trace:

- movingRoomList is referenced only by vessels_moving_rooms.[ch]; it is a
  discovery/countdown list, not an external API consumer. setup_moving_room
  appends loaded mover metadata and assigns world[rroom].mover.
- Replace its ten-second global scan with native room-owned recurring deadlines.
  Preserve resetZonePulse * ten seconds and RUN_ONCE lateness semantics; do not
  replay a backlog of relocations. The existing spec_gateway_moving_room remains
  the authored behavior entry point. Treat room index zero as valid.
- genwld.c free_room_strings does not free mover metadata; OLC copy and deletion
  require an ownership audit before dropping the global list. copy_room_with_bindings
  explicitly preserves existing runtime identities/affected-owner state. New
  mover ownership needs equivalent behavior across array shifts, editor clones
  and same-vnum replacement.

## Moving-room deadline checkpoint

Replaced movingRoomList/remainingZonePulses and service.moving_rooms with native
`moving-room.relocate` events owned by live room identities. Registration uses
the existing event_runtime; a single startup pass after world loading admits
loaded movers. There is no repeated discovery or idle global mover service.

Each callback resolves its room and checks the currently attached mover before
calling spec_gateway_moving_room. Existing callback arguments, return handling
and no_specials behavior remain intact. Index zero is valid. Deadlines retain
resetZonePulse * ten seconds; nonpositive resets retain the former minimum
ten-second cadence. Delayed dispatch performs one relocation and schedules the
next deadline from that dispatch, without replaying a backlog of moves.

Runtime handles live on room_data, not shared authored mover metadata. OLC
editor copies clear them, live same-mover edits preserve remaining time, room
deletion cancels them, and array shifts retain the stable room identity. Copies
of mover metadata to a different vnum are rejected rather than scheduling the
source mover under a new owner. The room diagnostic now reports native seconds
remaining, or -1 when unscheduled. Runtime shutdown cancels the active job list;
that list contains already-admitted timers and does not discover or count down
world rooms.

Existing mover metadata is still borrowed by OLC editor copies; this change
does not make editors owners of live timers or introduce metadata frees into
that shared lifetime. Metadata allocation/clone ownership remains an existing
separate concern; timer ownership is explicit and independently cleaned up.

Validation (2026-09-06): final `make -j10 test` passed with 1,163 gameplay
cases and all invoked integration checks, with no compiler warnings. Coverage
includes native callbacks, index zero, no_specials, OLC copies, stale identity,
delayed dispatch, array reindex and replacement during the running callback.
`make install` succeeded afterward. No game process was restarted.

Staff-event follow-up checkpoint:

- staff_event_tick is still called from point_update_global_one. It decrements
  the inter-event delay and active duration, ends expired events before running
  event-specific work, and maintains jackalope populations/prisoner portals.
- State accessors exist (set_event_state, clear_event_state, set_event_delay,
  get_event_time_remaining, get_event_delay), but diagnostics and command gates
  also read STAFF_EVENT_TIME/STAFF_EVENT_DELAY directly. Replace read-side
  countdown assumptions together with scheduling, including next_tick-based ETA.
- start_staff_event currently announces before setting duration and performing
  event-specific setup. Native admission must precede announcements and spawning
  so allocation failure cannot leave an announced, unscheduled event.
- Use existing native timers for expiry, inter-event delay, population and
  portal/environment agendas. Retain the shared mud-hour phase and make restart
  policy explicit; do not simply put staff_event_tick behind another periodic
  timer. End-event cleanup sets the delay, so guard callbacks with the event
  incarnation to prevent old cleanup or expiry affecting a replacement event.

Remaining assigned scope is still open: staff-event work in #105; #106-109
beyond the prior committed quest consumers; final #111 measurements; and
#112 release-gate evidence within the authorized non-production scope.

## Staff-event agenda checkpoint

Removed the staff_event_tick call from point_update_global_one and replaced
its counters with existing event_runtime types: staff-event.expiry,
staff-event.delay-ended, staff-event.jackalope-population and
staff-event.prisoner-presence. Maintenance bodies are separate active-event
agendas; no whole legacy tick wrapper or global event discovery loop remains.

Deadlines preserve the shared 75-second mud-hour phase. Expiry takes precedence
over maintenance at the same boundary, and late maintenance executes once.
The next delay/maintenance wakeup is aligned to the next world-hour boundary.
Read-side status and ETA use native remaining time rather than stale counters.
The raw ticks_left field now marks admitted active state/duration; public
remaining-time accessors are authoritative.

Start admits expiry and maintenance before announcements/spawning. End validates
the exact active event and clears its ownership before callbacks/cleanup. New
events remain gated by the cleanup delay. A failed cleanup timer admission logs
and retains a closed admission gate rather than admitting an immediate new event.
Native callbacks and spawn batches match the event incarnation; placement
notifications re-resolve newly spawned mobiles and portals before using them.
Missing prisoner portal rooms are rejected at start and handled during maintenance
and cleanup.

Active events are not serialized across reboot/copyover; shutdown cancels their
agendas. A fresh process retains the initial three-mud-hour startup delay. The
flat help entry, local development database and new deployable SQL entry describe
this policy. No production database was modified.

Final validation (2026-09-06): `make -j10 test` passed with 1,167 gameplay
cases and all invoked integration checks, with no compiler warnings. Coverage
includes hour alignment, expiry/cooldown, wrong-event termination rejection,
replacement isolation, admission failure before announcements/spawning, and
shutdown/startup-delay behavior. `make install` succeeded after the full run.
No game process was restarted.

Next tactical-work trace (#106):

- ready_action.c already reserves and expires readied normal attacks, subscribes
  to committed timed CastingStarted, and revalidates the exact activity/cast ID
  at queued execution. Extend those owners and allowances rather than creating
  another reaction queue.
- Counterspell mode exists in combat_modes.c as AFF_COUNTERSPELL, but the spell
  paths do not consume it; act.wizard.c explicitly describes it as doing nothing.
  FEAT_IMPROVED_COUNTERSPELL also exists. A new implementation must reconcile
  these existing surfaces instead of presenting a second incompatible mechanic.
- Counterspell identification/resources and instant-cast policy require an
  explicit recorded decision, followed by real casting tests. Designated-ally
  protection needs a concrete attack-attempt/outcome fact, since damage alone
  misses attacks that fail or are prevented.

Completion remains unproven for the entire batch. In addition to #106-109 and
#111/#112, return to the #103/#104 caller audit and verify compound movement/
transfer boundaries against newly migrated consumers before final sign-off.
The last #105 lifecycle/diagnostics checklist item also needs a full-batch audit.

## Tactical rules decision checkpoint (2026-09-06)

The #106 implementation contract is now recorded in
`docs/systems/COUNTERSPELL_AND_ALLY_READINESS.md`. It retains the existing ready
owner and native event queue, explicitly excludes instant casts from the timed
counterspell window, specifies identification/resource/competition rules, and
defines ally defense from a committed attack attempt including misses. It also
records concrete source findings: slot extraction can consume moon bonuses,
HIT_MISS includes rejected operations, and execution must bind the exact ready
incarnation as well as the exact cast ID. These determine the next implementation
steps; no counterspell or ally-defense runtime completion is claimed.

## Counterspell resource prerequisite (2026-09-06)

The preparation system now exposes `spell_prep_base_resource_check`, a
non-consuming query that shares normal extraction's resource selection. It
returns before moon debit, preparation/innate queue changes, preservation rolls
and extraction messages. Unlike the general cast-admission helper, it does not
grant a staff or at-will bypass. Its result is not a reservation: the reaction
must still revalidate and consume at execution.

The API deliberately accepts only a base spell, matching the #106 decision.
Tracing the existing metamagic calculation found that Inquisitor Spell Metamastery
can attach a cooldown while computing circle cost. Exposing arbitrary metamagic
through a supposedly pure resource probe would therefore be incorrect; the base
query passes METAMAGIC_NONE throughout that calculation. Normal extraction keeps
its existing metamagic and preservation behavior.

Validation: `make -j10 test` passed with 1,169 gameplay cases and no compiler
warnings, followed by successful `make install`. New production-linked tests
verify repeated probes without debit/output, moon-before-preparation consumption,
prepared recovery queue insertion, spontaneous exhaustion, and no staff resource
bypass. Runtime counterspell dispatch, ally-defense facts and the remaining #106
acceptance cases are still outstanding.

## Native counterspell implementation checkpoint (2026-09-06)

`ready counterspell <caster> on casting` now extends the existing ready-action
owner. It admits the standard-action reservation only after watches and expiry
are installed, observes a visible local timed cast, requires a perceptible
component and one Spellcraft check above 20, then queues the native ready event.
Execution revalidates eligibility, visibility, base resource and the observed
cast ID before committing resource use and cancelling that exact activity with
a COUNTERED reason. Extraction retains the existing resource-preservation rules.
Neither the counter's normal spell effect nor a second casting activity runs.

Ready execution now checks its native event ID before touching the current
reservation, matching the existing expiry protection. The new activity cancel
entry point rejects a replaced ID. Old counterspell-mode commands clear the
inert mode flag and direct players to READY; persisted affect/feat numbers remain
stable. Speech, hands, incapacitation, room and PvP restrictions apply. Instant
casts and psionic manifestation do not create an eligible counter opportunity.

Flat READIED-ACTION/READY/COUNTERSPELL help and the matching development database
entry were updated and verified, with deployable SQL in
`sql/components/help_counterspell_readiness.sql`. No production change occurred.

Validation: `make -j10 test` passed with 1,176 gameplay cases and no compiler
warnings, followed by successful `make install`. Seven new production-linked
reaction cases cover a real timed cast and one preparation, no spell effect after
countering, visibility loss without debit, same-spell replacement protection,
overdue native dispatch, no matching resource, silent/still observability and an
actual player instant cast. The initial instant fixture used an NPC, which cannot
cast instantly in the existing parser; it was corrected to exercise the real
player path. No server was restarted.

#106 remains open: implement designated-ally attack facts/reactions, and complete
the remaining competition, equal-deadline, admission-failure, extraction and
encounter-transition acceptance cases. The broader assigned-batch scope and
performance/release gates are unchanged.

## Committed attack boundary checkpoint (2026-09-06)

The normal `resolve_hit` path now publishes typed AttackCommitted after its
pre-operation legality checks and DG fight trigger, before combat consequences.
The payload records a monotonic attempt ID, generation handles for attacker and
defender, origin room and attack kind. Routing uses the defender subject, allowing
the planned protector subscription to remain local to one designated ally.
Queued special-action dispatch returns before this marker; normal nested strikes
receive their own attempt identities. Hit/miss is not inferred from HIT_MISS,
which also represents rejected commands.

This revises the initial completed-attempt design: the queued protector must bind
the attacker before a lethal strike removes the ally. Publishing the commitment
before consequences achieves that without delaying dead-ally subscription cleanup
until an outcome fact. Subscribers queue reactions; they do not attack inside
publication. The reaction still executes after the triggering strike returns.

The new callback boundary captures generation handles before the DG trigger and
revalidates participants, rooms, weapon identity/selection and projectile holder
before proceeding. It also revalidates after AttackCommitted observers. An
observer retiring the attacker therefore cannot leave this path using the old
borrowed character pointer. This is bounded validation of the new entry boundary,
not a claim that every older combat callback has been audited.

Validation: `make -j10 test` passed with 1,181 gameplay tests and no compiler
warnings; `make install` succeeded. Five production-linked cases exercise a real
miss, real hit, peaceful-room rejection, absent launcher/ammunition, and retirement
by the publication observer. Foundation registry checks also verify the new
event's name and payload size. Initial fixture failures were resolved: the type
registry now contains eleven foundation types, and direct-hit fixtures initialize
the production-required attack queues.

Next: subscribe designated-ally readiness to AttackCommitted, switch its watched
target from ally to attacker when claimed, preserve one reserved strike, and
complete DG/ranged/nested-strike and lifecycle acceptance coverage. #106 and the
full assigned batch remain open.

## Designated-ally readiness checkpoint (2026-09-06)

`ready attack on ally <ally> attacked` now admits one visible local group member
or owned NPC follower and reserves the existing standard action. Its defender-
scoped AttackCommitted subscription checks the current relationship, visibility,
melee eligibility and PvP rules, claims the first eligible attacker, and queues
one normal readied strike. Incoming damage resolves before the reaction. Misses
can trigger it, and later notifications neither retarget it nor dispatch a queued
special attack. There is no additional reaction allowance or global scan.

The event bus intentionally rejects subscription admission during publication.
Consequently target loss is handled through source-room subscriptions admitted
up front, filtering one generation identity: the ally before the trigger and the
attacker afterward. Character death and extraction include a source-room topic
for this concrete consumer. An ally-death notification after the trigger leaves
the queued retaliation intact; movement, death or extraction of the bound attacker
cancels it. Old callbacks must match the current bound identity before cancelling.

Validation: `make -j10 test` passed with 1,192 gameplay cases and no compiler
warnings, followed by successful `make install`. Eleven new production-linked
cases cover incoming hit/miss, one reserved strike, untouched queued special
attack, repeated attempts, relationship/visibility changes, rejection before
action cost, attacker movement/death/extraction, and ally death before versus
after the trigger. Lifecycle cases explicitly exercise typed notifications and
identity retirement; they do not prove every deep DG callback/extraction path.
The flat help, deployable SQL and development READIED-ACTION entry were updated
and verified. No production change or server restart occurred.

#106 still requires the remaining equal-deadline/competing-counter, admission-
failure, encounter-transition and deep DG/ranged/nested-strike coverage. Audit
counterspell verbal perception against AFF_DEAF as part of visibility/senses
acceptance. #107-109, final #103-105 audits and #111/#112 gates also remain open.

## Counterspell senses acceptance checkpoint (2026-09-06)

Counterspell's verbal perception path now respects AFF_DEAF. Visible somatic
components still permit identification by a deaf observer. Revalidation at native
execution also checks the required sense, so losing hearing after identifying a
verbal-only cast cancels the opportunity without consuming its spell resource.
The existing visible-caster and speech/room eligibility rules remain in force.

Four additional production-linked cases exercise these combinations, including
the positive hearing/verbal-only case. Successful counter cases now assert the
exact activity ID and CANCELLED/COUNTERED terminal fact. A fixed hit-point
expectation varied by one during the full suite; terminal state directly proves
countering versus completion without conflating independent health changes.
Flat help, deployable SQL and development help text describe the senses rule.

Validation: `make -j10 test` passed with 1,196 gameplay tests and no compiler
warnings; `make install` followed the run. #106 still needs the remaining
competition/equal-deadline, admission-failure, encounter-transition and deep
DG/ranged/nested-strike acceptance work. No assigned issue or release gate is
being marked complete by this checkpoint.

## Counterspell competition and clock acceptance checkpoint (2026-09-06)

Two additional production-linked scenarios prepare two counterers against one
real timed cast. They cover both admission orders and equal reaction deadlines,
including dispatch delayed beyond the complete target casting duration. Exactly
one preparation enters recovery, one remains available, both reservations finish,
and the target's exact activity reports cancellation by counterspell. The prior
single-counter overdue test was strengthened: its old fixed 15-pulse advance did
not clearly exceed the complete NPC cast duration, so it now advances beyond that
duration explicitly.

Native readiness-type unavailability tests now cover both admission stages.
Initial failure leaves no ready owner, restores the live subscription count and
preserves both the action and preparation. Failure to queue an already-admitted
counter preserves its preparation while retaining the standard-action cost.
These exercise scheduling failure cleanup; they are not an allocation-failure
injection test.

The established ready-attack semantic-round fixture now also admits counterspell.
It verifies survival past the old wall deadline after combat admission and expiry
before actions become available at the next semantic turn. This covers the
counterspell flag's participation in the common ready lifecycle.

Validation: `make -j10 test` passed with 1,201 gameplay tests, no compiler warnings,
and successful subsequent `make install`. No gameplay or help behavior changed
in this checkpoint. Remaining #106 work includes the targeted deep DG/ranged/
nested-strike audit and completion review; #107-109, final #103-105 audits and
#111/#112 gates remain open.

## Attack commitment DG and ranged acceptance checkpoint (2026-09-06)

Six production-linked tests now run real DG fight commands, nested total-defense
ripostes and physical ranged projectiles through the attack commitment boundary.
DG teleporting either participant, destroying the selected weapon, or destroying
the ammunition pouch aborts before publication. A nested riposte publishes its
own attempt identity. A ranged miss across adjacent rooms publishes once and
releases its projectile from the pouch to the target room or destruction.

The first ranged fixture unexpectedly hit because equipment recalculation cleared
its artificial hitroll penalty. A GDB hardware watch traced the one-point health
change to handle_successful_attack/damage_with_projectile; inspecting the roll
confirmed hitroll zero and a successful attack. The fixture now selects hitroll
after equipping. No combat mechanics were changed to accommodate the test.

Validation: `make -j10 test` passed all 1,207 gameplay tests without compiler
warnings; `make install` succeeded afterward. These tests strengthen #106's
commitment-boundary evidence. The broader counterspell acceptance review,
#107-109 implementation, final #103-105 audits and #111/#112 gates remain open.

## Turn-relative effect and hazard design checkpoint (2026-09-06)

Traced effect duration, semantic participant turns, bleeding, room clouds, wall
crossing and player affect persistence. The implementation contract is recorded in
`docs/systems/TACTICAL_EFFECT_CLOCKS_AND_HAZARDS.md`. It specifies clock ownership,
start/end phases, residual-time preservation, source identity, bounded exposure
accounting, pilots and required verification. Existing native owners remain the
scheduler integration. #107 is not implemented or accepted by this design commit.

The wall missing-caster branch makes a redundant second mag_damage call, but that
function's null guard returns zero. This was traced further and is not treated as
a demonstrated duplicate-damage or crash defect. No speculative wall repair was
made. Next implementation work is the short-defense phase/clock pilot, followed
by bleeding and recurring-save exposure with their old behavior paths removed.

## Character-lifetime turn clock checkpoint (2026-09-06)

Added a character-lifetime semantic turn serial and a read-only next-turn snapshot
in combat_encounters. The existing participant counter resets on re-admission;
the new serial survives that boundary and must be paired with character generation
for effect identity. During a turn callback, the snapshot points to the following
turn, including late dispatch. New mobiles reset the serial after prototype copy.

The production-linked regression covers overdue dispatch, remaining time, callback
queries, departure and re-entry. The existing offset-clock merge regression now
checks both characters' serials and remaining deadlines after merging. This is
required clock integration for #107, not a completed Defensive Casting migration.
Its activation, old decrement and PDCt persistence remain unchanged pending the
actual effect-owner integration.

Validation: the final `make -j10 test` passed 1,208 tests without compiler
warnings; subsequent `make install` succeeded. #107 implementation and the
remaining assigned batch are still open.

## Defensive Casting clock pilot checkpoint (2026-09-06)

Wizard Defensive Casting now uses the existing native scheduler outside combat
and its subject's next semantic turn when freshly activated in combat. The old
proc_d20_round_one decrement is removed. A running elapsed interval is preserved
on combat admission; leaving combat converts the next turn's residual time into
a native expiry rather than restarting six seconds. Payloads carry generation
handles and expire only the current event ID. Shutdown captures semantic clocks
before membership destruction; character removal pauses/cancels native expiry.
A link-dead character remaining in world continues to expire normally.

PDCt persists a residual pulse interval alongside the legacy display-round field.
Old one-field saves still load; an explicitly zero residual cannot resurrect a
fresh round. No runtime event handle, pulse deadline or turn serial is written.
New mobile instances clear the runtime clock fields after prototype copying.

Seven production-linked cases cover exact native expiry, the removed decrement,
expiry before semantic actions, residual combat departure, pause/resume, a live
character without a descriptor, legacy loading, actual residual save/load and
expired saved state. Some cases cover multiple invariants. Flat help, development
DB help and sql/components/help_defensive_casting_clock.sql agree on the rules.
Both build manifests include the new native owner source.

Validation: final `make -j10 test` passed 1,215 tests without compiler warnings;
`make install` succeeded afterward. Remaining #107 work includes further
lifecycle/admission/merge acceptance, bleeding and recurring saves, and committed
hazard exposure. The assigned batch and final measurement/release gates remain
open; this checkpoint does not mark #107 complete.

## Defensive Casting transition acceptance checkpoint (2026-09-06)

Five additional production-linked cases cover refreshing through the original
expiry deadline, entering combat with an elapsed interval, leaving/re-entering
without restarting that interval, shutdown capture of semantic residual time, and
defense expiry across merged offset encounters. The merge test exercises both
participants: the original participant expires on its turn, while the absorbed
participant retains the bonus until its own deferred semantic turn. Elapsed
intervals retain their wall-time remainder and are not aligned to a later turn.

Validation: `make -j10 test` passed all 1,220 tests without compiler warnings;
`make install` succeeded. No production behavior or help changed in this
checkpoint. Admission/lifetime edge cases still require the final #107 audit.
Next implementation is Bleeding Critical's end-of-turn damage and duration as one
owner, excluding the migrated nodes from both legacy duration and AFF_BLEED
behavior processing. The separate SKILL_BLEEDING_ATTACK regeneration path is not
implicitly included in that pilot.

## Bleeding Critical clock pilot checkpoint (2026-09-06)

Migrated ordinary Bleeding Critical damage and duration together onto native
out-of-combat ticks and the subject's end-of-turn boundary. The original save,
damage roll, self-attribution and affect_join stacking remain; reapplication
preserves the pending tick while replacing duration and adding damage. Legacy
duration and AFF_BLEED callbacks exclude the migrated nodes. Other bleeding
sources remain outside this pilot.

Damage processing copies values, charges the interval before callbacks and then
resolves character generation/clock version. Cure or replacement during damage
cannot expire a replacement or replay the paid interval. Equal native/combat
deadlines are charged once in either callback order. Leaving combat during an
action preserves an already-due end tick with a one-pulse deadline.

The real save path temporarily strips affects. It now separately preserves the
live bleeding clock and writes BlCt residual pulses alongside the existing affect
record. Registry detach marks the character non-live before owner removal so
refill cannot re-admit it during detachment. Help agrees in the flat file,
development database and sql/components/help_bleeding_critical_clock.sql.

Validation: 11 new production-linked cases; final `make -j10 test` passed all
1,231 tests without compiler warnings, followed by successful `make install`.
Admission exhaustion/retry telemetry and remaining lifetime cases still need the
final #107 audit. Recurring saves, movement hazards, #108/#109 implementation and
the remaining assigned-batch audit/performance/release gates remain open.

## Room-effect late-dispatch checkpoint (2026-09-06)

Corrected the native room owner so elapsed lifetime boundaries are charged when
dispatch occurs after an exact cadence pulse. Hazard behavior remains run-once
per dispatch, and expiry is processed before behavior. Per-source lifetime state
prevents a newly admitted room affect from inheriting overdue rounds from an
older source in the same room. Three production-linked regressions cover late
expiry, non-burst behavior and fresh-source isolation.

This is required timing groundwork for #107's recurring-save pilot. Billowing
cloud source identity, subject exposure accounting and semantic end-of-turn saves
remain open.

## Billowing Cloud exposure checkpoint (2026-09-06)

Migrated Billowing Cloud from the room-wide behavior loop to source-owned,
per-character exposure intervals. Committed movement handles entry, character-
owned native events handle continued exposure outside combat, and the subject's
semantic end phase handles it in combat. One next-due value prevents duplicate
entry, re-entry and turn checks in the same interval and survives transitions
between native and semantic timing.

Each cloud has its own identity and a bounded exposure list. Admission failure is
counted and rejects the untracked exposure. Source removal cancels future events;
world-lifetime checks make expiry win even when another event at the same deadline
dispatches first. The old Billowing Cloud case in room_aff_tick is removed, while
the level threshold, Fortitude save and action consequence remain.

Production-linked coverage includes forced entry, same-interval re-entry, native
continuation, combat entry and exit, vanished sources, exact-deadline expiry,
distinct overlapping sources, level immunity and capacity rejection. Flat help,
development database help and the deployment SQL component describe the rules.
Crossing hazards, tentacles and final #107 lifecycle/capacity acceptance remain
open.

Validation: final `make -j10 test` passed all 1,241 tests without compiler
warnings, followed by successful `make install`.
