# Assigned issue repair batch

Base: `origin/master` at `55d19e0bd`, branch `fix/open-issue-repairs`.
Scope: open issues assigned to `jamclaug` on 2026-09-06, #103 through #112.
The wider unassigned queue is excluded. No production rollout or archival deletion is authorized.

## Implementation checkpoint

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

- [ ] Replace descriptor polling for crafting (`craft_update`) and self-buff sequences with owned activities; preserve cancellation, material costs, timing and offline policy.
- [ ] Replace transport `travel_tickdown` with passenger/job deadlines; transit need not occupy a passenger's primary activity.
- [ ] Give supply refresh an explicit online/offline policy and next deadline or justified lazy timestamp.
- [ ] Replace `movingRoomList` countdown discovery with owned mover deadlines.
- [ ] Give active staff events named agendas for delay, expiry, portals and population replenishment.
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
