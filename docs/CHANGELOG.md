# Changelog

## [Unreleased] - July 30, 2026

### Vessel system - shared development harbor

Built the persistent shared harbor fixture required for repeatable builder,
ferry, interior-script, restart, and multiplayer vessel validation.

#### Added

- A development-only, idempotent harbor provisioner with two static seaports,
  raft/ship/airship prototypes, a looping two-stop route, a public scheduled
  ferry, and a persistent NPC ferrymaster. It refuses non-development
  environments and never runs from normal install or deployment.
- Phase 11 schema support for attaching DG trigger prototypes to generated
  room types through `ship_room_template_triggers`.
- `vedit spawnpublic <id>` for unclaimed NPC/public hulls that do not accrue
  player-owner dock fees.
- A one-login `--vessel-builder-check` that derives live IDs from game output,
  creates, tunes, shows, and spawns through `vedit`, proves the generated hull
  can sail, and removes its disposable ship and prototype.

#### Fixed

- Pilot assignment/removal, schedule creation/removal, and scheduled departure
  now report success only after persistence succeeds. Failed writes restore
  prior memory and compensate any partial durable change.
- The harbor world-data merger inserts missing records in VNUM order while
  preserving existing builder-authored records.
- `shippurge` now retains the full vessel name in its operator confirmation
  and log instead of copying it through the shorter player-name buffer.
- Assigned vessel pilots at the bridge no longer take ordinary random mobile
  exits. The harbor ferrymaster is also Sentinel, and an actively staffed helm
  prevents the unmanaged-helm structural hit during a gale.
- `shipfix` now persists the repaired runtime condition before reporting
  success and restores the prior condition if that write fails.

#### Validated

- An idempotent local run restored the moving ferry, active hourly schedule,
  route progress, and ferrymaster across a hard restart. Both generated-room
  DG triggers fired, and the east dock loaded as seaport room 1000390 at
  `(-62, 82)`.
- Kohdee completed the no-C builder workflow in 2.7 seconds in game and 8
  seconds including login, cleanup, character logout, and account logout. The
  generated Boat sailed from `(-66, 92)` to `(-67, 92)`, and all temporary
  data was removed.
- During a 60-second live storm check, the ferrymaster remained at the bridge
  through multiple hazard pulses and the repaired ferry stayed 20/20 on all
  sides. A full service restart retained the repair and restored the pilot.

### Documentation - vessel source-of-truth consolidation

Distilled the completed design and implementation knowledge from the temporary
vessel PRD into the maintained documentation tree. The temporary vessel
workspace now contains unfinished work only.

#### Changed

- Replaced the stale root vessel PRD with the durable product vision, design
  pillars, player and staff outcomes, wilderness contract, quality budgets,
  scope boundaries, release scorecard, and principal risks.
- Updated the vessel architecture decision and behavior reference with the
  shared-wilderness invariants, corrected memory evidence, persistence
  lifecycle, operational gates, and known limitations.
- Recast the benchmark document as an evidence record: the 4,744-byte ship
  measurement and historical test results remain, while the complete
  500-ship, 25 ms benchmark and 72-hour soak are explicitly unverified.
- Corrected the manual regression guide to record the live step-3 legacy
  fleet-slot blocker instead of calling that path fixed.
- Rewrote the schema runbook around all shipped phases, authoritative help,
  snapshot rehearsal, property census, reverse-order destructive rollback, and
  staged rollout.
- Replaced stale considerations that claimed production readiness, a
  1,016-byte ship structure, and a standalone test runner.
- Replaced the obsolete completion-era documentation audit with a current
  source-of-truth map and evidence rules, and expanded the vessel incident
  runbook with safe containment and recovery boundaries.

#### Removed

- The temporary final vessel PRD after its durable content was incorporated.
  `VESSELS_TODO.md` is now the sole file in the vessel workspace and holds the
  dependency-ordered live backlog.

### Vessel system - local end-to-end validation and release boundary

Completed the local-development regression with the actual level-34 Kohdee
character and converted the development controls, help, and recovery paths
from planning items into tested behavior.

#### Added

- A command feature flag for the complete vessel, vehicle, transport,
  autopilot, builder, and player command surface. Cedit `Vessel System: Off`
  now stops those commands plus both heartbeat tick groups. Bulletin-board
  `board` behavior and staff diagnosis/recovery commands remain available.
- Production-safe debug controls: support defaults to compiled out;
  an explicit `-DVESSEL_SYSTEM_DEBUG=1` development build starts with an empty
  runtime mask controlled by `vdebug`/`vesseldebug` across ten categories.
- An idempotent authoritative help migration with 31 maintained entries and 75
  exact command keywords, plus a read-only verifier for counts, access levels,
  nonempty content, and obsolete duplicates.
- Fast, marker-delimited Kohdee command batches, deterministic editor dialogs,
  pager-safe help checks, and a one-login exhaustive vessel help sweep in the
  local login helper and guide.
- A Phase 09 runtime schema, verifier, and rollback for complete live vessel
  snapshots and schedules. The Kohdee helper now has a descriptor-preserving
  `--copyover-check` mode with optional pre-copyover commands.
- A Phase 10 schema, verifier, and rollback for normalized installed weapons,
  durable insurance claims, opponent-specific PvP logout grace, and dock-fee
  state.
- One class-based dock fee per owned port visit, `dockfees [pay]`, clan-port
  revenue, public-port gold sinks, and departure/autopilot blocking until
  settlement. Unowned NPC and test hulls remain exempt.
- Durable insurance claim mail and one-time login delivery for offline owners,
  plus a transactional permanent-player-removal policy that unowns vessels,
  removes permits, and voids pending claims.

#### Fixed

- Canonical fleet-slot identity, active-slot detection, the zone-700 fixture,
  generated-room ordering and reindexing, route deletion and persistence,
  vehicle enumeration/transport/persistence, docking and hostile boarding,
  and immediate generated-interior reclamation on purge or sink.
- Legacy and builder-spawned hulls now share complete armor, internal
  structure, rigging, and steering initialization. This prevents a legacy hull
  with 100 armor but zero structure from sinking on its first absorbed weather
  hit.
- Boot now relinks zone-reset hull objects to active fleet slots. A character
  saved in static vessel room 70003 can disembark normally after a full
  restart without boarding again.
- Prototype-spawned ships now survive graceful reboot and copyover with their
  dynamic interiors, exterior hulls, coordinates, heading, speed, condition,
  combat link, weapon-slot state, route progress, schedules, ownership, cargo,
  crew, upgrades, and insurance. Interior saves use an upsert that preserves
  child rows instead of `REPLACE` cascade deletion.
- Copyover commits vessel state before writing the descriptor handoff and
  aborts safely if persistence fails. Its post-`chdir` handoff-file diagnostic
  now checks the actual `lib/copyover.dat` location instead of logging a false
  path-mismatch `SYSERR`.
- Stale duplicate help mappings no longer shadow canonical vessel topics;
  general character movement speed remains available through
  `MOVEMENT-SPEED` and `RACIAL-SPEED`.
- Port checks now follow authoritative wilderness coordinates across recycled
  dynamic rooms. Dock debt cannot be bypassed after copyover or restart, and
  payment remains available at the recovered berth.
- Player `setroute` and autopilot on/off/pause/resume controls now persist the
  runtime row before reporting success. A failed write restores the exact
  previous route and state instead of leaving RAM and MariaDB divergent.
- Persisted hull recovery now treats wilderness coordinates as authoritative,
  keeps object-only dynamic rooms occupied, and splits generated names into
  readable boarding keywords. Every active slot reconstructs a runtime hull,
  while zone reset cleanup preserves hulls already managed by the fleet.

#### Validated

- All 30 numbered vessel regression steps passed on local development with
  Kohdee, including sailing, generated interiors, route restart, vehicles,
  docking, hostile boarding, purge, same-slot reuse, and cleanup.
- A live autopilot route held the exact same coordinates across separate
  Kohdee sessions while the cedit kill switch was off. Gated commands refused,
  ordinary play and recovery commands remained available, and enabling the
  option restored tick processing.
- An explicit debug build emitted only the requested movement category; the
  restored default build reported debug support compiled out. The root
  production-linked suite passed 220 tests and was installed with no
  root-level `circle` artifact.
- A dynamic transport carrying cargo, crew, a refit, insurance, ownership, and
  combat damage, plus a damaged scheduled warship mid-route, retained all
  expected state through both a graceful full restart and a real copyover. The
  active warship recovered in `Traveling` state and continued moving on the
  same route while Kohdee's descriptor stayed connected.
- An owned transport incurred one 35-gold charge on arrival, could not depart
  while indebted, retained that debt through copyover and restart, paid it,
  departed, returned, and incurred exactly one new charge for the new visit.
- The Goshawk reproduced stale Traveling-state recovery after an abrupt local
  process replacement. With the fix installed, immediate SQL and subsequent
  hard replacements retained Off, assigned-route, Paused, Traveling, and
  disengaged states exactly. A targeted MariaDB failure injection rejected a
  resume write and both memory and SQL remained on the prior Paused state.
- Three hulls co-located at Testing Dock and two hulls co-located in one
  dynamic wilderness room remained visible and independently boardable through
  hard restart and `zreset 10000`. The unattended dynamic room stayed
  occupied, boot relinked all five hulls, readable `board` keywords worked,
  and purging the temporary fifth hull left no persistence.
- Veska bought a 50-gold policy, logged out at 9,990 gold, and received one
  pending claim and one underwriter receipt when Kohdee sank the raft through
  actual gunfire. Her next login showed 10,040 gold and a paid claim; a second
  login kept the same balance and created no duplicate.
- Corven's reversible deleted flag blocked login while preserving a raft deed
  and Tern helm permit; restoration returned Corven aboard the same raft. The
  actual fast-wipe character-menu deletion then removed the player, made the
  raft unclaimed, removed the permit, and voided a controlled pending claim.
- A temporary MariaDB trigger forced Elyra's vessel cleanup transaction to
  fail during the actual password-confirmed deletion flow. Deletion was
  cancelled before account unlinking, the trigger was removed, and Elyra
  immediately logged back in with her raft deed and separate Tern helm permit
  intact.
- Dorrin and Elyra established a consented vessel engagement, then Elyra
  logged out. Dorrin's descriptor and opponent-specific grace survived
  copyover, Veska was refused as a third party, and Dorrin was refused after
  the full five-minute window. The run found that deed transfer cleared grace
  only in memory; deed and capture now commit owner plus runtime reset
  together, while permanent removal clears it inside the cleanup transaction.
  A fresh-boot live deed confirmed owner `Elyra` with zero/empty grace in SQL.
- All 75 help keywords resolved to database help in one 54-second Kohdee
  session, all SQL help checks passed, and all 21 current component migrations
  applied independently to a fresh MariaDB 10.11 master schema. Cleanup left
  no test vehicles, prototypes, routes, waypoints, or runtime ship-instance
  rows.

### Documentation - structured web onboarding promotion

Promoted the completed web account and character-creation project from the
Zusuk scratch workspace into maintained system documentation. The permanent
reference is `docs/systems/WEB_ONBOARDING_SYSTEM.md`.

#### Added

- A source-traced reference for the structured onboarding presentation
  adapter: authority boundaries, descriptor lifecycle, reserved MSDP
  variables, v1/v2 coverage, bounded state documents, source-owned catalogs,
  private editor transfer, checked role-play persistence, privacy, web
  performance and accessibility, media ownership, compatibility, activation,
  rollback, testing, and maintenance.

#### Changed

- Replaced the stale account and character-creation examples in
  `PLAYER_MANAGEMENT_SYSTEM.md` with the current `nanny()` flow, active
  male/female and no-point-buy creation contract, alignment save boundary,
  optional role-play profile, checked versus legacy save semantics, and
  source-authority rules.
- Linked the structured pre-game protocol from `PROTOCOL_SYSTEMS.md` and added
  the default/v2 clean-build matrix to `TESTING_GUIDE.md`; the focused parser
  harness now records its reserved v2 action-dispatch coverage.
- Preserved the durable media-key, fallback, privacy, accessibility,
  performance, provenance, and cross-repository validation contracts in the
  canonical system reference. The live asset checklist remains owned by
  `luminariweb/docs/manifest.md`.
- Updated the documentation indexes.

#### Removed

- `WEB_ACCOUNT_CHARACTER_CREATION_EXPERIENCE.md`, whose implemented design and
  durable operating knowledge are now covered by the canonical references.
  Dated estimates, phase checklists, commit hashes, PIDs, binary hashes, and
  rollout narration were intentionally not promoted.
- The source-side `manifest.md`, which was an older duplicate of the web
  client's actively maintained asset and delivery checklist.

## [Unreleased] - July 27, 2026

### Artifact System - chronicle, provenance, and the second-wave roster

Built the public-facing and content-contract half of the artifact system, and
added the six complete HomelandMUD candidate artifacts as native LuminariMUD
content. Behavior and operations are documented in
`docs/systems/ARTIFACT_SYSTEM.md`; remaining packaging, live-placement,
integration-test, and balance work is tracked in
`docs/project-management-zusuk/ongoing-projects/artifacts.md`.

The replica or "echo" model from the HomelandMUD study was considered and
rejected. One object VNUM per artifact remains the rule, and template VNUM
uniqueness is now validated at boot.

#### Added

- **Public artifact chronicle.** `artifact roster` shows every artifact
  available in the running campaign as `unawakened`, `unclaimed`, `held`,
  `lost`, or `recoverable`, derived from the registry on every call.
  `artifact chronicle <name>` gives the fuller entry. Names are withheld
  until an artifact has been discovered, the acquisition hint stays out of
  the record until somebody has actually found one and let it go, and the
  current bearer is named only where that artifact's contract makes bearers
  public - otherwise the chronicle names the first bearer, which is history.
  No room number or VNUM ever appears.
- **Provenance and custody history**, stored separately from current
  ownership and read by nothing that decides anything: first bearer and
  account, first and last claim times, and claim, transfer, destruction,
  recovery, and staff-override counts.
- **Acquisition and release policy** in `artifact_contracts[]`: acquisition
  type, campaign availability, owner-visibility policy, one line of public
  lore, and one line of acquisition hint per artifact.
- **Group-targeted artifact powers.** `ART_TARGET_GROUP_ROOM` acts on the
  invoker and every eligible same-room group member, selected before any
  effect runs. One cooldown and one XP award per activation; an invocation
  that reaches nobody refuses and costs nothing.
- **A reusable signature-proc library**: controlled knockdown with saves and
  immunity rules, wounded-heal versus healthy-offense, alignment-conditioned
  ward and dispel, weighted multi-outcome, bounded combat surge, and bounded
  extra-attack burst. Each shape carries an alignment rule. New artifacts
  select a shape rather than adding a function.
- **Proc stacking groups.** Temporary artifact powers in the same group never
  stack; the running one holds and the second refuses at no cost.
  Doombringer's rage and Twilight's surge share a group.
- **Data-driven invocation channels.** An effect declares whether it answers
  to `say`, `whisper`, or the new `invoke` command. One matcher serves all
  three, and the phrase, channel, displayed help, and runtime dispatch all
  come from the same table row. An effect never answers on another channel.
- **Progressive passive powers** in `artifact_passives[]`: senses, haste,
  protections, and saving-throw grants that unlock by artifact level, applied
  as source-tagged affects rather than prototype flag bits. `artifact info`
  shows both the active and the still-locked ones.
- **Boot-time metadata validation** covering templates, contracts, effect
  rows, and passive rows, run again by `testartifact verify`. It logs a
  precise `SYSERR` naming the offending row and disables only the invalid
  effect, never the registry.
- **Audited artifact recovery.** `testartifact recover <vnum>` is the one
  sanctioned way to return a lost artifact to play. It refuses while a live
  instance exists and for an unowned artifact, states whose ownership it
  overrides, preserves provenance, counts the recovery, and logs it.
- **Six new artifacts** at VNUMs 169913-169918: Vengeance, Earthcrier,
  Wyrmfang, Courage, Icedge, and Twilight. Identity, lore, and the shape of
  their powers come from the HomelandMUD study; every mechanic is rebuilt on
  Luminari's own damage, affect, saving-throw, and progression rules.

#### Changed

- **Ownership file format v2.3.** Adds the custody history and every cooldown
  stamp - active ability, generic proc, and each called-effect slot - so a
  restart no longer hands every power back. v1, v2.0, v2.1, and v2.2 files
  still load; records are distinguished by field count. Loading an older file
  marks an owned artifact discovered but invents no first bearer.
- **`scripts/provision_artifacts.sh` now merges.** It still never overwrites
  a deployed world file, because a builder may have edited it through OLC,
  but it now adds object prototypes and zone resets the live files do not
  have yet. Repeated runs remain no-ops.
- `testartifact list` reports chronicle state and acquisition type.
- Doombringer's `enrage me doombringer` now uses the shared stacking group
  and bounded, source-tagged affects.

#### Fixed

- **Amaukekel's group recall.** `sunlit path to paradise` recalled the caller
  before iterating the group, so each member's real location was compared
  against the caller's destination and every ordinary nearby group member was
  silently skipped. The origin room and the member list are now snapshotted
  before anyone moves.
- **`testartifact spawn` no longer duplicates a durably owned artifact.** It
  previously accepted any VNUM with no live instance, including one whose
  owner was merely offline, and room placement then cleared
  `instance_persisted` so the next zone reset could create a third. A refused
  spawn now changes nothing.
- **`testartifact reload` no longer discards deferred state.** It flushes
  dirty registry state before rebuilding; `reload discard` is the explicit
  opt-out.

## [Unreleased] - July 27, 2026

### Artifact System - persistent unique items and content layer

Introduced a native LuminariMUD artifact system and completed the
RealmsOfLuminari content port. The permanent behavior and operations reference
is `docs/systems/ARTIFACT_SYSTEM.md`; unfinished deployment, placement,
integration-test, balance, cooldown-persistence, validation, and runtime
hardening work is tracked in
`docs/project-management-zusuk/ongoing-projects/artifacts.md`.

#### Added

- **Compile-time artifact registry** for eleven unique items in reserved zone
  1699. Membership is a successful VNUM lookup in a boot-sorted registry, so
  object identity and artifact data cannot drift between separate indexes.
- **Persistent ownership and progression** in
  `lib/world/world.artifact`. The v2.2 writer records owner, account, level,
  cumulative XP, bind time, and durable-instance state through a temporary
  file plus atomic rename. The loader retains explicit compatibility with ROL
  v1/v2.0 and LuminariMUD v2.1 records.
- **Complete object-lifecycle integration** for direct and recursively nested
  artifacts: acquisition, release, room placement, equip, unequip, player
  save extraction, actual destruction, reload reassociation, and house
  storage. Bound items retain their owner when dropped, while unbound items
  return to circulation.
- **Single-instance enforcement** on all four object-loading zone reset
  commands (`O`, `P`, `G`, and `E`). Live instances and durably stored owned
  instances block respawn; a bound artifact lost in an ordinary room remains
  recoverable after reboot.
- **Four binding modes**: unbound, bind on pickup, bind on equip, and bind on
  account. Account binding uses the real account identity, and staff at
  `LVL_IMMORT` or above can bypass binding for operations and testing.
- **Level-scaled equipment bonuses** for all six ability scores, hit roll, damage
  roll, AC, hit points, PSP, and movement. Artifact affects are source-tagged
  so removing one item cannot strip another artifact's bonuses.
- **Highest-only artifact resistance** for physical, magical, and elemental
  damage categories.
- **Five-level progression** with cumulative thresholds of 100, 300, 600,
  and 1000 XP. XP comes from first equip, NPC hits and kills, critical and
  boss-tier combat, procs, abilities, and called effects. Generic combat XP
  lands on one random equipped artifact rather than multiplying across every
  item worn.
- **Player interface**:
  `artifact [list|info <item>|progress|abilities|help]`, including generated
  bonuses, ownership, binding, oath, active ability, called phrase, cooldown,
  and progression output.
- **Three active abilities** with PSP costs and cooldowns:
  `soulstrike` from Kelrarin's Hammer, `divineward` from Amaukekel, and
  `doomblast` from Doombringer. Hostile selection uses the existing
  `aoeOK()` gate, and Doom Blast does not spend resources when no valid target
  exists.
- **Generic artifact weapon procs** for soul damage, self-healing, fear,
  doom damage, and a level-5 NPC execute, protected by a real 30-second
  per-artifact internal cooldown.
- **Five signature weapon procedures**:
  Trorxek's critical-hit blindness; Kelrarin's returning lifesteal throw and
  alignment-gated mega blast; Kelrom's animal taboo and group healback;
  Gesen's returning Harm strike; and Avernus's emergency full heal.
- **Eighteen speech-invoked effects** across Trorxek, Amaukekel, Fade, the
  Horn of Henekar, and Doombringer. `say` input is normalized for case,
  whitespace, and common trailing sentence punctuation; each effect has an
  independent one-hour through one-week recharge stamp.
- **Native equivalents for upstream content** including Oaken Defender
  summoning, creeping doom, recall, guarded travel-to-player, and a
  group-recall path; resurrection, dispel evil, blindness, darkness,
  enfeeblement, room
  pacification, capped NPC charm, room annihilation, black lightning, and
  rage.
- **Class oaths** requiring ten Druid, Cleric, Rogue, or Warrior levels for
  five artifacts. A mismatched wearer takes one `5d4` fire burn per update
  and cannot invoke or identify called effects.
- **Staff operations** through
  `testartifact <status|verify|save|reload|spawn|list|reset>`, including
  owned/dropped/unowned reporting, live location, duplicate detection,
  registry memory accounting, and targeted ownership reset.
- **Artifact deployment tooling** through the idempotent
  `scripts/provision_artifacts.sh`, called by setup and deployment, plus the
  reserved Vault of Ages room, eleven object records, artifact help, and
  Oaken Defender package contract. The source package is currently ignored
  by Git and remains an explicit follow-up before clean-clone deployment.
- **Production-linked regression coverage** in
  `unittests/CuTest/test_artifacts.c`. The file now contains 45 artifact test
  functions, including 15 added for the content layer, recharge behavior,
  speech refusal paths, oath checks, dropped-state accounting, critical XP,
  and one-recipient generic combat XP.

#### Changed

- Threaded critical-hit state and triggering damage from
  `handle_successful_attack()` into artifact combat and signature-proc paths.
- Defined boss-tier artifact XP as an NPC at least three levels above its
  attacker, activating the existing x2 hit and x3 kill multipliers while
  retaining level-scaled base kill XP.
- Hooked called effects into ordinary `say` processing and oath burns into
  `point_update()`.
- Rebuilt the upstream special-procedure content as a data-driven called
  effect table plus explicit signature-proc dispatch, without importing its
  incompatible `SPECIAL()` event framework.
- Replaced the upstream `obj->cost = -1` temporary-copy marker with an
  explicit player-save extraction scope and a locationless-clone guard.
- Removed the disabled, non-building
  `src/specs.artifacts.c`/`src/specs.artifacts.h` upstream paste and removed
  its source entry from both Autotools and CMake. All live behavior resides
  in `src/world/spec_artifacts.c`.
- Consolidated enduring design, gameplay, persistence, deployment,
  operations, extension, and testing details in
  `docs/systems/ARTIFACT_SYSTEM.md`. The developer workspace file now contains
  unfinished work only.
- Versioned the initial persistent system at 2.5014-beta and the completed
  content layer at 2.5016-beta, synchronizing CMake, Autotools, the runtime
  version string, and README at each step.

#### Fixed

- Bound artifacts no longer rewrite their owner when another character picks
  them up, closing a complete binding bypass.
- Bind-on-account artifacts now set their bind timestamp on first wear.
- Removing one artifact strips only that artifact's affects; it no longer
  removes every `SPELL_ARTIFACT_BONUS` affect.
- Level-up reapplies bonuses immediately instead of waiting for re-equip.
- The generic proc internal cooldown is read and enforced, rather than merely
  written.
- Ability costs come from the artifact template and display correctly in
  `artifact info`.
- A single whole-file writer replaces incompatible fixed-position and
  whole-file persistence paths.
- Generic hit and kill XP no longer multiplies across all equipped artifacts.
- Critical-hit and boss-tier XP constants are now connected to combat.
- Class restrictions use one shared reachable gate rather than the
  unreachable per-procedure path in the upstream implementation.
- Oaken Defender summoning uses a dedicated local prototype instead of an
  unreachable branch and campaign-dependent VNUM.
- Artifact help entry terminators use `#0`, eliminating help-loader minimum
  level errors on the machines carrying the world package.
- Dropped-state scanning uses a separate container cursor and no longer
  corrupts the outer `object_list` traversal.

#### Verification

- Clean GNU C23 build with no new compiler warnings.
- Production-linked CuTest suite: 133/133 passing.
- `make install` updated `bin/circle` and removed the root-level server
  artifact.
- Boot verification initialized all eleven artifacts, loaded v2.2 state,
  reset zone 1699 without artifact `SYSERR` output, and confirmed Oaken
  Defender prototype loading.

## [Unreleased] - July 26, 2026

### Documentation - Zusuk workspace audit

Audited every document in `docs/project-management-zusuk/`, which is developer
scratch space rather than a home for official documentation. Each file was
classified by verifying its claims against the code, not by trusting its own
status headers.

#### Changed

- **Finished work with enduring value moved into the formal documentation tree:**
  - `current-casting-visuals.md` -> `docs/systems/CASTING_VISUALS_SYSTEM.md`
  - `client_capabilities_and_player_preferences.md` -> `docs/systems/CLIENT_CAPABILITIES_AND_PREFERENCES.md`
  - `MOUNT_AUDIT.md` -> `docs/systems/MOUNT_SYSTEM.md`
  - `char-rename-fixes.md` -> `docs/systems/CHARACTER_RENAME_SYSTEM.md`
  - `INTEGRATION_GUIDE.md` -> `docs/systems/INTERMUD3_GATEWAY_API.md`
  - `CIRCLEMUD_CLIENT_AUDIT.md` -> `docs/systems/INTERMUD3_SECURITY_AUDIT.md`
  - `casting-visuals-testing.md` -> `docs/testing/CASTING_VISUALS_TESTING.md`
  - `overview.md` -> `docs/guides/LUMINARI_OVERVIEW.md`
  - `PRODUCTION_DEPLOYMENT_STEPS.md` -> `docs/deployment/VESSEL_SCHEMA_DEPLOYMENT.md`,
    generalized from Phase 2 to all vessel phases with corrected
    `sql/components/` paths
- **Unfinished work consolidated** in `project-management-zusuk/ongoing-projects/`
  with a README stating each item's real status: AI conversation history (not
  started), SKORE phases 3-4, protocol security follow-ups (4 of 6 open), the
  event-system merge (not started), 76 outstanding CMake format warnings, and the
  player/staff idea backlog.
- **`docs/systems/CASTING_VISUALS_SYSTEM.md` brought up to date.** It documented
  the pre-enhancement system: no mention of schools, class styles, or
  environmental reactions, and no reference to `casting_visuals.c` at all, even
  though the seven-phase enhancement shipped 2025-11-26. It now documents all five
  delivered feature families, their entry points, target-type and message-slot
  model, and the fallback behavior when a message table entry is missing.
- **`docs/systems/INTERMUD3_GATEWAY_API.md` gained a scope note.** Its paths
  (`clients/python/i3_client.py`, `docs/API_REFERENCE.md`) refer to the external
  gateway project's repository, not this codebase, and never resolved here.
- **`docs/TECHNICAL_DOCUMENTATION_MASTER_INDEX.md`** updated for every move, with
  new entries for the protocol, mount, casting-visuals, rename, and I3 documents.

#### Removed

Three documents whose value was fully consumed, verified before deletion:

- `DEPLOYMENT_LOG.md` - a dated session log from 2025-11-20. Every build error it
  recorded is fixed (the `mariadb/mysql.h` include, the undeclared `REGION_*`
  constants), and its top recommendation - a CI pipeline to catch build errors -
  now exists as `.github/workflows/`.
- `casting-visuals/ideas-casting-visuals.md` and
  `casting-visuals/improving-casting-visuals.md` - all five ideas and all seven
  implementation phases shipped, confirmed present in `src/casting_visuals.c`.
  Their content was folded into the system documentation first.

#### Noted

- `docs/testing/vessel_test_results.md` describes eight test files
  (`test_vessels.c`, `test_vessel_coords.c`, `vessel_stress_test.c`,
  `test_runner.c`, and others) that **no longer exist** - they were standalone
  mirror suites, since replaced by the production-linked CuTest suite. Marked as a
  historical Phase 00 record rather than deleted, since it is not this project's
  document to remove.

### Vessel System - Phases 04 through 09 (gameplay layer)

Turns the vessel system from working transport infrastructure into a gameplay
system: ships can be authored by builders, bought and owned by players, crewed,
upgraded, insured, fought, sunk, plundered, and used to run cargo and freight
across the wilderness. Every environmental signal comes from the existing
wilderness system - no vessel-private geography, weather, or terrain.

Documentation: `docs/systems/VESSEL_SYSTEM.md` (behavior reference),
`docs/testing/VESSEL_SYSTEM_TESTING.md` (30-step manual regression script),
`docs/testing/VESSEL_BENCHMARKS.md` (memory attribution and test figures), and
`docs/PRD.md` (durable requirements, wilderness contract, and release criteria).
Outstanding work is isolated in
`docs/project-management-zusuk/vessels/VESSELS_TODO.md`.

#### Added

- **Builder tooling** (`src/vessels_edit.c`) - `vedit` ship prototype editor
  (`list`/`new`/`show`/`set`/`delete`/`spawn`, LVL_BUILDER) backed by a new
  `ship_prototypes` table. `vedit spawn` produces a live boardable ship:
  allocates a fleet slot, applies per-prototype class/speed/armor, generates the
  interior, wires the object linkage, and persists immediately.
- **Data-driven ship interiors** - `ship_room_templates` rows override the
  compiled-in room template array at boot (`load_ship_room_templates_from_db()`),
  so builders change generated interiors without recompiling. Compiled-in
  templates remain as the MySQL-unavailable fallback.
- **Naval combat** (`src/vessels_combat.c`) - per-side armor absorption with
  spill into section internal structure and bleed-through from destroyed
  sections; damage bands (sound/battered/crippled/sinking); subsystem
  degradation (bow hits tear rigging and cut speed, stern hits foul the rudder);
  `vessel_sink()` evacuates all interior rooms into the water, converts the ship
  object to salvageable wreckage, and frees the fleet slot.
  - `shipfire <slot> <target>` with range gating, firing-arc gating, d20 +
    gunnery attack rolls against a speed-derived defense, damage dice from slot
    data, and reload timers on the heartbeat (`vessel_combat_tick()`).
  - `greyhawk_getarc()` implemented (it was previously declared but never
    defined) - computes the facing arc from heading-relative bearing.
  - NPC auto-defense doctrine: NPC-piloted ships track their last attacker and
    return fire with every bearing, loaded weapon.
  - `shiprepair` (at-sea repairs, stationary only) and `claimship` (capture from
    an uncontested bridge).
  - Groundings: sailing into water shallower than the hull's draft, measured
    against real wilderness bathymetry, halts the ship and damages the bow.
- **Ownership and shipyards** (`src/vessels_ownership.c`, `src/vessels_edit.c`) -
  player ownership persisted on `ship_interiors.owner`; helm permits (up to 10
  names) persisted in `ship_crew_roster`; `shipbrowse`/`shipbuy`/`shipchristen`/
  `shipdeed`/`shippermit`/`shiprevoke`/`shipcrew`.
- **Hired crew** (`src/vessels_crew.c`) - four positions (sailmaster, gunner,
  bosun, quartermaster) at three quality tiers, with signing costs and recurring
  wages; `shiphire`/`shipdismiss`/`shipwages`. Bonuses are written into the
  legacy `sailcrew`/`guncrew` fields so they feed existing systems: speed in
  movement, accuracy in gunnery, repair rate, and cargo capacity. Unpaid wages
  accrue and eventually cost you a crew member.
- **Refits, wear, and insurance** (`src/vessels_upgrades.c`) - `shipupgrade`
  installs plating (+50% armor), rigging (+5 speed), hold (+25% cargo), or
  reinforcement (+50% structure); `vessel_upkeep_tick()` wears armor and
  subsystems while under way; `shipinsure` buys coverage capped at hull value,
  paid out from `vessel_sink()`.
- **Bulk cargo and port trading** (`src/vessels_trade.c`) - `trade_commodities`
  (9 seeded goods, builder-editable) and `port_commodities` per-port supply
  seeded deterministically from port vnum so ports differ without randomness.
  `market`/`cargobuy`/`cargosell`/`cargomanifest`. Price scales with local
  scarcity, hard-clamped to +/-60% of base; buying drains local stock and selling
  floods it; ports buy at 85% of ask so same-port round trips lose money.
- **Freight contracts** (`src/vessels_contracts.c`) - per-port boards with TTL
  refresh; `contracts`/`contractaccept`/`contractdeliver`/`contractabandon`.
  Offers are generated from live commodity prices and real wilderness distance
  between dock rooms, and only to ports that actually trade.
- **Piracy and bounty** (`src/vessels_piracy.c`) - `plunder` transfers cargo
  from a cleared prize to an alongside raider; `bounty` and `marque`. Unlawful
  plunder accrues bounty, and `vessel_port_refuses()` is enforced at every
  port-service entry point, so a WANTED pirate cannot sell what they steal. A
  letter of marque legalizes prizes for a real day.
- **Weather hazards and encounters** (`src/vessels_hazards.c`) - storm severity
  bands read from the shared wilderness weather field (squall/storm/gale degrade
  rigging; a gale with no sailmaster aboard damages the hull); fog closes
  visibility, a posted lookout reopens it; submarine crush depth checked against
  real bathymetry; submerged submarines sheltered from surface weather;
  `seastate` reports water, depth, weather, visibility, and hull state.
  - Encounter engine: `vessel_encounters` rows key to `REGION_ENCOUNTER`
    wilderness regions via `get_enclosing_regions()`, filtered by depth band and
    hull class, with lookout warnings. Creatures spawn into the ship's
    wilderness room so they fight, flee, and take fire like anything else.
- **Operator tooling and client protocol** (`src/vessels_admin.c`) - `shiplist`
  (fleet overview plus wilderness dynamic room pool utilization, flagged past
  80%), `shipgoto <slot>`, `shipfix <slot>`. New MSDP variables pushed to anyone
  aboard a ship: `SHIP_NAME`, `SHIP_X`, `SHIP_Y`, `SHIP_Z`, `SHIP_HEADING`,
  `SHIP_SPEED`, `SHIP_HULL`, `SHIP_HULL_MAX`, `SHIP_STATUS`.
- **Debug instrumentation across the vessel and vehicle stack** - a
  production-off compile-time switch (`VESSEL_SYSTEM_DEBUG` in
  `src/vessels.h`) plus a ten-category runtime mask, so an explicit development
  build can trace one subsystem without noise from the rest. Categories cover
  core, movement, autopilot, docking, database, function tracing, state
  transitions, vehicles, vehicle movement, and vessel transport; every line
  carries a greppable `[VESSEL_*]` or `[VEHICLE_*]` prefix.
  - Instrumentation completed across all nine files: `vessels_docking.c` (~30
    calls), `vehicles_transport.c` (all 8 functions), `vessels.c` (position
    updates, terrain checks, speed modifiers, blocked moves, room allocation),
    `vessels_autopilot.c` (start/stop/pause/resume, tick summary, travel steps),
    `vehicles.c` (state transitions, damage, terrain verdicts), plus
    `vessels_rooms.c`, `vessels_db.c`, `vehicles_commands.c`, and
    `transport_unified.c`.
  - Reference (categories, macro names, grep recipes) is documented in
    `docs/systems/VESSEL_SYSTEM.md` under Troubleshooting -> Debug Logging.
  - Normal builds compile diagnostics out. `vdebug`/`vesseldebug` controls the
    empty-by-default mask only when support was explicitly compiled into a
    development build.
- **Per-class cargo capacity** - `get_vessel_cargo_capacity()` data table plus
  `vessel_effective_cargo_capacity()`, which folds in the hold refit and the
  quartermaster's stowage bonus.
- **Authoritative vessel help** - `help_vessel_entries.sql` maintains 31
  database entries covering all 75 exact vessel, vehicle, transport,
  autopilot, and staff-recovery command keywords. Staff-only surfaces use
  min-level 31.
  - `verify_help_vessel_entries.sql` checks entry and command counts, access
    levels, nonempty text, and removal of obsolete duplicate mappings.
  - Ignored standalone `.hlp` files are not indexed or maintained sources. A
    deployment that needs file fallback should provision one consolidated
    `help.hlp` from authoritative content.
  - Verified through the running game: every command keyword returned a
    database `Help Tag`, including ambiguous navigation and movement terms.
- **SQL components** - schema, rollback, and verify scripts for each phase:
  `vessels_phase4_*`, `vessels_phase6_*`, `vessels_phase7_*`, `vessels_phase8_*`
  in `sql/components/`. The Phase 08 verify script flags any encounter row whose
  region is missing or is not actually a `REGION_ENCOUNTER` region.
- **Tests** - production-linked CuTest coverage in
  `unittests/CuTest/test_transport_production.c`: cargo capacity table, combat
  status bands, firing arcs including heading offset, armor absorption and
  sinking, an NPC-vs-NPC duel harness, helm permission matrix, shipyard pricing,
  crew costs and bonuses, upgrade effects, trade price bounds swept across the
  entire supply domain, cargo weight accounting, lookout/weather behavior, the
  PvP consent gate, and stale attacker-reference cleanup on sinking.
  Suite grew from 60 to 74 tests, all passing.

#### Changed

- **Helm access on owned ships** - `is_pilot()` now requires owner, helm permit,
  or immortal on an owned vessel. Unowned vessels (test hulls, unclaimed
  ferries) remain open to anyone.
- **Vessel tick** - `comm.c` now drives `vessel_combat_tick()`,
  `vessel_crew_wage_tick()`, `vessel_upkeep_tick()`,
  `vessel_trade_restock_tick()`, `vessel_weather_tick()`,
  `vessel_encounter_tick()`, and `vessel_msdp_tick()` alongside the existing
  autopilot tick.
- **Boot sequence** (`db.c`) - loads DB room templates and ensures the
  ownership, trade, contract, piracy, and encounter schemas.
- **Ship movement** - speed now credits the sailmaster's handling bonus (capped
  at the hull's maximum) and runs a grounding check after each move.
- **`NUM_VESSEL_TYPES`** moved from `vessels.c` to `vessels.h` next to the
  `vessel_class` enum so other modules can bounds-check against it.
- **Vessel documentation consolidated and relocated out of the developer
  workspace.** `docs/project-management-zusuk/` is scratch space, not a home for
  official documentation, so the enduring vessel docs moved into the formal tree:
  - `VESSEL_SYSTEM.md` -> `docs/systems/` (alongside COMBAT_SYSTEM.md,
    CLAN_SYSTEM.md, and the other system references), updated with every new
    command and subsystem. Its Debug Logging section was rewritten to document
    the actual macro system - it previously showed hand-rolled `log()` calls that
    did not reflect the code.
  - `VESSEL_MANUAL_TEST.md` -> `docs/testing/VESSEL_SYSTEM_TESTING.md`, renamed
    to match the directory's convention (RESOURCE_SYSTEM_TESTING.md,
    WEATHER_INTEGRATION_TESTING.md) and rewritten as a 30-step numbered
    regression script.
  - `VESSEL_BENCHMARKS.md` -> `docs/testing/`, with the corrected memory
    attribution.
  - The working vessel PRD was later distilled into permanent product,
    architecture, system, testing, and benchmark documentation. The workspace
    now retains only `VESSELS_TODO.md`, containing unfinished work.
  - Two working documents were retired entirely once their content landed in
    permanent homes: `VESSEL_CHECKLIST.md` and `todo.md` (the debug logging
    tracker) - completed work to this changelog, outstanding work to
    `VESSELS_TODO.md`.
  - Inbound links updated in TECHNICAL_DOCUMENTATION_MASTER_INDEX.md,
    adr/0001-unified-vessel-system.md, and CONSIDERATIONS.md.

#### Fixed

- **Cargo capacity was never enforced** - `check_vessel_vehicle_capacity()` had
  a stubbed weight check, so `loadvehicle` accepted any load. It now totals
  loaded vehicle weight against the vessel's effective capacity.
- **23 stale "TODO Session 02/03" comments** in `vessels_autopilot.c` sat above
  working code and misrepresented the file's state. Resolved: routes now get
  unique session-local ids (negative, so they cannot collide with database
  AUTO_INCREMENT ids), and `route_save()`/`route_load()` genuinely round-trip
  through the `ship_routes`/`ship_waypoints` tables with idempotent waypoint
  replacement. Vessel sources are now TODO-free.
- **Boarding gaps** - `setup_boarding_defenses()` now repositions idle NPC crew
  to the entrance and bridge chokepoints; a failed hostile boarding drops the
  character into the actual wilderness water room with a d20 + Athletics swim
  check following the `movement_validation.c` convention; the boarding roll now
  factors in Athletics rather than level alone.
- **Format-string risk in room generation** - builder-authored template strings
  were passed to `snprintf` as format strings. Replaced with an explicit
  single-substitution helper that treats them as literal text.
- **Damage could not sink an evenly-matched ship** - damage to a destroyed hull
  section was absorbed instead of bleeding through to the rest of the hull, so
  two comparable ships could pound each other indefinitely. Caught by the
  NPC-vs-NPC duel harness.
- **`vessel_sink()` leaked** the autopilot and schedule allocations attached to
  the ship. Also caught by the duel harness.
- **Vessel combat bypassed the MUD's PvP consent system** (the most serious
  defect found). `pk_allowed` is enabled in this installation, but `pvp_ok()`
  (`src/utils.c`) additionally requires *both* players to have `PRF_PVP` set,
  arena excepted - and no vessel code called it. Consequences, all now fixed via
  a new `vessel_pvp_permitted()` gate that resolves a ship's owner and routes
  them through `pvp_ok()`:
  - `shipfire` could sink a non-consenting player's ship, drowning her crew and
    destroying her cargo.
  - `plunder` could steal a non-consenting player's cargo.
  - `claimship` could seize a moored ship with no combat whatsoever - board a
    ship, walk to the bridge, take it while the owner was logged off.
  - `do_board_hostile` (pre-existing, Phase 02) forced combat via `set_fighting()`
    on every player in the boarded room; `set_fighting()` does not gate either.
    Defenders are now checked individually, so a passenger who has not enabled
    PVP is not dragged into a fight for standing on deck.
  - Unowned hulls (test vessels, unclaimed NPC ferries) remain fair game, staff
    can always act, and an owner who is not logged in cannot consent - so their
    ship is protected while they are away.
- **Sunk ships left stale attacker references** - `last_attacker` holds a fleet
  slot index, and `vessel_sink()` frees the slot for reuse without clearing other
  ships' references to it. The next hull created in that slot inherited the
  grudge and would be fired on unprovoked by NPC return fire. `vessel_sink()` now
  clears the index fleet-wide.
- **Memory budget documentation was wrong** - the benchmarks doc and the PRD
  recorded 1016 bytes per ship with a 2KB cap. The struct actually measures 4744
  bytes, and was already ~4400 before this work; Phases 04-09 added roughly 340
  bytes. The dominant cost is legacy (`desc[256]` inside each of ten equipment
  slots). At 500 ships the fleet costs 2.3 MB, which is negligible, so the
  documented budget was corrected to 5KB/ship rather than restructuring working
  Greyhawk display code. Full component attribution is in the benchmarks doc.

#### Technical Details

- **New files**: `vessels_admin.c`, `vessels_combat.c`, `vessels_contracts.c`,
  `vessels_crew.c`, `vessels_edit.c`, `vessels_hazards.c`,
  `vessels_ownership.c`, `vessels_piracy.c`, `vessels_trade.c`,
  `vessels_upgrades.c` - all registered in both `Makefile.am` and
  `CMakeLists.txt`.
- **New tables**: `ship_prototypes`, `trade_commodities`, `port_commodities`,
  `freight_contracts`, `vessel_bounties`, `vessel_encounters`. Extended
  `ship_interiors` with `owner`, `upgrades`, `insured_for`, `wages_owed`
  (auto-migrated at boot). Activated the previously unused `ship_crew_roster`
  and `ship_cargo_manifest` tables.
- **Wilderness integration** (the design constraint throughout): groundings and
  crush depth read `get_modified_elevation()` against `wild_waterline`; weather
  reads the shared `get_weather(x, y)` field, so a storm at sea is the same
  storm a coastal walker experiences; encounters key to wilderness regions via
  `get_enclosing_regions()`; freight distances derive from dock room
  coordinates. No new coordinate space, terrain table, or weather source was
  introduced. `shiplist` monitors the shared dynamic room pool, which vessels
  borrow from rather than own.
- **Quality gates**: build clean with no new warnings under `-Wall -Wextra`;
  74/74 production-linked tests pass; valgrind reports 0 definitely/indirectly/
  possibly lost bytes and 0 errors; the whole system remains behind the existing
  cedit vessel-system toggle.
- **Verification status**: all of the above is code-complete and test-verified,
  but has not been exercised on a running server. Live-server verification
  (harbor content, soak testing, tick budget measurement, player beta) remains
  outstanding - see the Remaining Work section of the PRD.

# [Unreleased] - October 10, 2025

### Minimal World Bootstrap & Database Hardening

#### Added
- **Starter area documentation** (`docs/world/STARTER_AREA.md`) covering the minimal three-room loop aligned with start VNUMs.
- **`pet_data` schema creation** in the bootstrap initializer so fresh databases persist companion stats.

#### Changed
- Replaced the stub world definition with a sorted, four-room minimal world (`lib/world/(minimal|wld)/0.wld`) including a defined `#0` fallback.
- Widened the minimal zone range to `0-3099` so start rooms resolve cleanly on boot.
- MSDP room updates now guard against invalid room indices to avoid formatting crashes during login/movement.

#### Fixed
- Movement and MSDP segfaults caused by empty start rooms and invalid wilderness lookups.
- Repeated pet load/save SQL warnings by provisioning the missing `pet_data` table during setup.

## [Unreleased] - August 28, 2025

### Deployment System - Complete Overhaul (100% Complete)

#### Added
- **Simple setup script** (`scripts/simple_setup.sh`) - Zero-interaction deployment in under 2 minutes
  - Automatic configuration file setup from examples
  - Build system execution with error handling
  - Symlink creation for world/, text/, and etc/ directories
  - World file initialization with proper index naming
  - Text file creation for all required game files
  - Directory structure setup for player files

#### Fixed
- **Deploy script path navigation** - Script now correctly navigates from scripts/ to project root
- **World file copying bugs** - Fixed incorrect wildcard usage that copied wrong index files
- **Index file naming** - Files are now properly renamed from `index.xxx` to `index`
- **Symlink creation** - Automatically creates required symlinks (world, text, etc)
- **HLQ directory** - Added missing Homeland Quest directory and index
- **Text file initialization** - All required text files now created automatically

#### Changed
- **Deployment workflow** - Simplified from complex manual process to single script execution
- **Error handling** - Graceful MySQL bypass when database not configured
- **Documentation** - Updated all deployment guides with working instructions

## [Unreleased] - August 26, 2025

### Intermud3 Integration - Complete Repair and Enhancement (100% Complete)

#### Added
- **Complete Intermud3 client implementation** - Full thread-safe inter-MUD communication system
  - `src/systems/intermud3/i3_client.c` - Core threaded client with event queuing (901 lines)
  - `src/systems/intermud3/i3_client.h` - Complete API definitions and data structures (215 lines)
  - `src/systems/intermud3/i3_commands.c` - All player and admin commands implemented (602 lines)
- **Thread-safe architecture** - Producer-consumer event queuing between I3 thread and main game thread
- **Complete command set** - All inter-MUD communication features:
  - `i3tell <user>@<mud> <message>` - Send tells across MUD network
  - `i3chat [channel] <message>` - Multi-MUD channel communication
  - `i3who <mud>` - Query remote MUD player lists
  - `i3finger <user>@<mud>` - Get remote player information
  - `i3locate <user>` - Search for users across network
  - `i3mudlist` - List all connected MUDs on network
  - `i3channels list|join|leave [channel]` - Channel subscription management
  - `i3config` - Toggle server-wide I3 features on/off (immortal only)
  - `i3admin status|stats|reconnect|reload|save` - Administrative functions
- **JSON-RPC 2.0 protocol compliance** - Full implementation of I3 Gateway protocol
- **Configuration system** - File-based configuration in `lib/i3_config`
- **Event processing integration** - Seamless integration with main game heartbeat

#### Fixed
- **All critical security vulnerabilities** identified in CircleMUD client audit:
  - Buffer overflow vulnerabilities in message processing
  - Use-after-free and memory corruption issues  
  - Format string vulnerabilities in logging
  - Threading safety violations and race conditions
  - Resource leaks in socket and memory management
- **Complete protocol implementation** - All previously stub functions now fully implemented:
  - `i3_request_who()` - Remote player list queries
  - `i3_request_finger()` - Remote player information
  - `i3_request_locate()` - Cross-network user location
  - `i3_request_mudlist()` - Network MUD directory
  - `i3_join_channel()` / `i3_leave_channel()` - Channel management
  - `i3_send_emoteto()` / `i3_send_channel_emote()` - Emote support

#### Changed
- **Thread safety implementation**:
  - Proper mutex usage for all shared data structures
  - Event queuing prevents cross-thread character_list access
  - Safe message passing between I3 thread and main thread
- **Memory management**:
  - Proper JSON object cleanup with `json_object_put()`
  - Bounds checking on all string operations using `strncpy()`
  - Resource tracking and cleanup on shutdown
- **Error handling**:
  - Comprehensive input validation and sanitization
  - Graceful handling of connection failures with auto-reconnect
  - Proper error propagation and logging
- **Build system integration**:
  - Added to both `Makefile.am` and `CMakeLists.txt`
  - All commands registered in `interpreter.c`
  - Main loop integration in `comm.c` heartbeat function

#### Technical Details
- **Architecture**: Event-driven design with thread-safe producer-consumer queues
- **Dependencies**: json-c library for JSON-RPC protocol, pthread for threading
- **Performance**: Non-blocking I/O, efficient queue operations, minimal main thread impact
- **Security**: Input validation, bounds checking, safe string operations throughout
- **Configuration**: `lib/i3_config` with gateway host, port, API key, and feature toggles

#### Testing and Documentation
- **Integration status document** created at `docs/systems/narrative-weaver/INTERMUD3_INTEGRATION_STATUS.md`
- **Updated audit report** reflects successful remediation of all security issues
- **Production readiness**: Full compliance with I3 Gateway specifications
- **Testing instructions**: Comprehensive guide for verifying functionality

### Vessel System - Phase 2 Progress (80% Complete)

#### Added
- **Multi-room vessel interiors** - Ships now support 1-20 dynamically generated interior rooms
  - `vessels_rooms.c` - New file implementing room generation, templates, and connections (573 lines)
  - `vessels_docking.c` - New file implementing docking and boarding mechanics (412 lines)
- **Room template system** - 10 different room types with dynamic descriptions:
  - Bridge, Quarters, Cargo Hold, Engineering, Weapons, Medical, Mess Hall, Corridor, Airlock
- **Vessel-specific room generation** - Different vessel types get appropriate room layouts:
  - Warships: Multiple weapon rooms, engineering
  - Transports: Multiple cargo holds
  - Smaller vessels: Fewer, more compact layouts
- **Ship-to-ship docking mechanics**:
  - `dock` command - Dock with nearby vessels
  - `undock` command - Separate from docked vessels
  - Automatic gangway creation between docked ships
  - Safety checks for proximity and speed
- **Combat boarding system**:
  - `board_hostile` command - Attempt hostile boarding
  - Skill-based success checks
  - Consequences for failure (damage, falling into water)
- **Interior viewing commands**:
  - `look_outside` - View wilderness from ship interior
  - `ship_rooms` - List all rooms in current vessel
- **Room connection algorithm** - Smart hub-and-spoke layout with cross-connections

#### Changed
- **Build system integration**:
  - Updated Makefile.am to include vessels_rooms.c and vessels_docking.c
  - Updated CMakeLists.txt with new source files
- **C89/C90 compatibility fixes**:
  - Changed all `number()` calls to `rand_number()`
  - Fixed room coordinate fields to use `coords[0]` and `coords[1]`
  - Corrected `damage()` function calls with proper parameters
  - Fixed room_flags array handling with SET_BIT_AR
  - Moved all variable declarations to block start (no C99 loop declarations)

#### Fixed
- Compilation errors related to undefined functions
- Type mismatches for room coordinate fields
- Incorrect damage() function parameters
- C99 loop variable declarations
- Room flags assignment for array-based flags

#### Technical Details
- **Files modified**: vessels.h, Makefile.am, CMakeLists.txt
- **New dependencies**: spells.h (for TYPE_UNDEFINED)
- **Functions implemented**:
  - `generate_ship_interior()` - Creates room layout based on vessel type
  - `complete_docking()` - Establishes connections between ships
  - `do_dock()`, `do_undock()` - Docking commands
  - `do_board_hostile()` - Combat boarding command
  - `do_look_outside()` - View external wilderness

#### Remaining Work
- Database persistence for ship configurations
- Interior movement integration with ship navigation
- NPC crew management
- Cargo transfer system completion
- Performance optimization
- Unit test suite creation
- Integration testing with live gameplay
